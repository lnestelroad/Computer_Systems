#include <stdio.h>
#include "cs1300bmp.h"
#include <iostream>
#include <fstream>
#include "Filter.h"

using namespace std;

#include "rdtsc.h"

//
// Forward declare the functions
//
Filter * readFilter(string filename);
double applyFilter(Filter *filter, cs1300bmp *input, cs1300bmp *output);

int
main(short int argc, char **argv)
{

  if ( argc < 2) {
    fprintf(stderr,"Usage: %s filter inputfile1 inputfile2 .... \n", argv[0]);
  }

  //
  // Convert to C++ strings to simplify manipulation
  //
  string filtername = argv[1];

  //
  // remove any ".filter" in the filtername
  //
  string filterOutputName = filtername;
  string::size_type loc = filterOutputName.find(".filter");
  if (loc != string::npos) {
    //
    // Remove the ".filter" name, which should occur on all the provided filters
    //
    filterOutputName = filtername.substr(0, loc);
  }

  Filter *filter = readFilter(filtername);

  double sum = 0.0;
  short int samples = 0;

  for (short int inNum = 2; inNum < argc; inNum++) {
    string inputFilename = argv[inNum];
    string outputFilename = "filtered-" + filterOutputName + "-" + inputFilename;
    struct cs1300bmp *input = new struct cs1300bmp;
    struct cs1300bmp *output = new struct cs1300bmp;
    short int ok = cs1300bmp_readfile( (char *) inputFilename.c_str(), input);

    if ( ok ) {
      double sample = applyFilter(filter, input, output);
      sum += sample;
      samples++;
      cs1300bmp_writefile((char *) outputFilename.c_str(), output);
    }
    delete input;
    delete output;
  }
  fprintf(stdout, "Average cycles per sample is %f\n", sum / samples);

}

struct Filter *
readFilter(string filename)
{
  ifstream input(filename.c_str());

  if ( ! input.bad() ) {
    short int size = 0;
    input >> size;
    Filter *filter = new Filter(size);
    short int div;
    input >> div;
    filter -> setDivisor(div);
    for (short int i=0; i < size; i++) {
      for (short int j=0; j < size; j++) {
	short int value;
	input >> value;
	filter -> set(i,j,value);
      }
    }
    return filter;
  } else {
    cerr << "Bad input in readFilter:" << filename << endl;
    exit(-1);
  }
}


double
applyFilter(struct Filter *filter, cs1300bmp *input, cs1300bmp *output)
{

  long long cycStart, cycStop;

  cycStart = rdtscll();

  output -> width = input -> width;
  output -> height = input -> height;

    //pull out cache memory calls from the for loops
    const short int Iheight = (input -> height) - 1;
    const short int Iwidth = (input -> width) - 1;
    
    // pull out function call from for loop
    const short int filterSize = filter -> getSize();
    short int filterDiv = filter -> getDivisor();
    
    //put filter int o a local array to reduce calls to cache memeory.
    short int tempOutput[3];
    short int tempFilter[3][3];
    for (short int j = 0; j < filterSize; j++) {
        for (short int i = 0; i < filterSize; i++) {	
            tempFilter[i][j] = filter -> get(i,j);
        }
    }

//   for(short int plane = 0; plane < 3; plane++) {
        for(short int row = 1; row <  Iheight; row++) {
            for(short int col = 1; col < Iwidth; col++) {

            tempOutput[0] = 0;
            tempOutput[1] = 0;
            tempOutput[2] = 0;

//             for (short int j = 0; j < 3; j++) {
                const short int coll1 = col - 1;
                const short int coll2 = col;
                const short int coll3 = col + 1;
                
//                 for (short int i = 0; i < 3; i++) {
                    const short int roww1 = row - 1;
                    const short int roww2 = row;
                    const short int roww3 = row + 1;
                
                
                    //unrolled r = 0
                    tempOutput[0] += (input -> color[0][roww1][coll2] * tempFilter[0][1] );
                    tempOutput[1] += (input -> color[1][roww1][coll2] * tempFilter[0][1] );
                    tempOutput[2] += (input -> color[2][roww1][coll2] * tempFilter[0][1] );
                    
                    tempOutput[0] += (input -> color[0][roww1][coll1] * tempFilter[0][0] );
                    tempOutput[1] += (input -> color[1][roww1][coll1] * tempFilter[0][0] );
                    tempOutput[2] += (input -> color[2][roww1][coll1] * tempFilter[0][0] );
                    
                    tempOutput[0] += (input -> color[0][roww1][coll3] * tempFilter[0][2] );
                    tempOutput[1] += (input -> color[1][roww1][coll3] * tempFilter[0][2] );
                    tempOutput[2] += (input -> color[2][roww1][coll3] * tempFilter[0][2] );
                    
                    //unrolled r = 1
                    tempOutput[0] += (input -> color[0][roww2][coll2] * tempFilter[1][1] );
                    tempOutput[1] += (input -> color[1][roww2][coll2] * tempFilter[1][1] );
                    tempOutput[2] += (input -> color[2][roww2][coll2] * tempFilter[1][1] );
                    
                    tempOutput[0] += (input -> color[0][roww2][coll1] * tempFilter[1][0] );
                    tempOutput[1] += (input -> color[1][roww2][coll1] * tempFilter[1][0] );
                    tempOutput[2] += (input -> color[2][roww2][coll1] * tempFilter[1][0] );
                    
                    tempOutput[0] += (input -> color[0][roww2][coll3] * tempFilter[1][2] );
                    tempOutput[1] += (input -> color[1][roww2][coll3] * tempFilter[1][2] );
                    tempOutput[2] += (input -> color[2][roww2][coll3] * tempFilter[1][2] );
                    
                    //unrolled r = 2
                    tempOutput[0] += (input -> color[0][roww3][coll2] * tempFilter[2][1] );
                    tempOutput[1] += (input -> color[1][roww3][coll2] * tempFilter[2][1] );
                    tempOutput[2] += (input -> color[2][roww3][coll2] * tempFilter[2][1] );
                    
                    tempOutput[0] += (input -> color[0][roww3][coll1] * tempFilter[2][0] );
                    tempOutput[1] += (input -> color[1][roww3][coll1] * tempFilter[2][0] );
                    tempOutput[2] += (input -> color[2][roww3][coll1] * tempFilter[2][0] );
                    
                    tempOutput[0] += (input -> color[0][roww3][coll3] * tempFilter[2][2] );
                    tempOutput[1] += (input -> color[1][roww3][coll3] * tempFilter[2][2] );
                    tempOutput[2] += (input -> color[2][roww3][coll3] * tempFilter[2][2] );
                    
//                   }
//             }
	
            tempOutput[0] /= filterDiv;
            tempOutput[1] /= filterDiv;
            tempOutput[2] /= filterDiv;

            if ( tempOutput[0]  < 0 ) {tempOutput[0] = 0;}
            if ( tempOutput[0]  > 255 ) {tempOutput[0] = 255;}
                
            if ( tempOutput[1]  < 0 ) {tempOutput[1] = 0;}
            if ( tempOutput[1]  > 255 ) {tempOutput[1] = 255;}
                
            if ( tempOutput[2]  < 0 ) {tempOutput[2] = 0;}
            if ( tempOutput[2]  > 255 ) {tempOutput[2] = 255;}

            output -> color[0][row][col] = tempOutput[0];
            output -> color[1][row][col] = tempOutput[1];
            output -> color[2][row][col] = tempOutput[2];
          }
        }

  cycStop = rdtscll();
  double diff = cycStop - cycStart;
  double diffPerPixel = diff / (output -> width * output -> height);
  fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
	  diff, diff / (output -> width * output -> height));
  return diffPerPixel;
}
