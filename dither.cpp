#include <list>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

#include "CImg-1.5.7/CImg.h"
using namespace cimg_library;

#include "lib/ColorHist.h"
#include "lib/Color.h"
#include "lib/Triplet.h"


/** 
    This program loads a palette image and an image and then try to
    display this image using the computed palette and a given
    threshold matrix.
    Use : 'dither %paletteFile %thresholdPattern %imageFile (%outputFile)'
    Parameters :
    paletteFile : The path to the palette file.
    thresholdPattern : The path to an image used as a threshold matrix
    imageFile : The path to the image file to dither
    outputFile : The result image will be saved under this name
    Result :
    The reconstructed image will be saved.
*/


inline Color getColor(int const x, int const y, CImg<unsigned char> const& image) {
    return Color(image(x, y, 0), image(x, y, 1), image(x, y, 2));
}

/** Parse a palette image and return a pointer to the Color array
    containing all the colors. Assign the number of colors to the colorCount parameter. */
Color * parsePalette(CImg<unsigned char> const& paletteImage, int& colorCount) {
    // Initialize the number of colors
    colorCount = 0;

    // Init the color array
    Color * palette = new Color[paletteImage.height()*paletteImage.width()];

    // Parse the palette image and grab all the colors !
    for(int y = 0; y < paletteImage.height(); y++) {
	for(int x = 0; x < paletteImage.width(); x++) {
	    // If not transparent
	    if(paletteImage(x, y, 3) == 255) {
		// Add the color
		palette[colorCount] = getColor(x, y, paletteImage);
		// That's one more color :)
		colorCount++;
	    }
	}		     
    }

    cout << "Color count : " << colorCount << endl;

    return palette;    
}

/** Return an non dithered image, made with the input palette */
CImg<unsigned char>  nodither(CImg<unsigned char> const& image, CImg<unsigned char> const& paletteImage) {
    // The resulting image, reconstructed from the original with the palette's colors
    CImg<unsigned char> reconstructed(image.width(), image.height(), 1, 3, 0);

    // Temporary color variable
    Color c;
    // The nearest color index
    int nearest;
    // Distance of the nearest color
    long nearestDist;
    // Distance between two colors
    long dist;

    // The palette colors array
    Color * palette;
    // The number of colors in the palette
    int colorCount;
    
    // Grab the colors from the palette and set the colorCount
    palette = parsePalette(paletteImage, colorCount);

    // For each pixel in the image, find the best color
    for (int y = 0 ; y < image.height() ; y++) {
	for (int x = 0 ; x < image.width() ; x++) {
	    // The color to approximate
	    c = getColor(x, y, image);
	    // The current nearest color is the first...
	    nearest = 0;
	    nearestDist = 16777216; // (256*256*256) = maximum value
				    // of distance2(color1, color2)
	    // Find the closest color in the palette
	    for (int k = 0 ; k < colorCount ; k++ ) {
		dist = Color::distance2(c, palette[k]);
		if (dist < nearestDist) {
		    nearestDist = dist;
		    nearest = k;
		}
	    }
	    // Set the target pixel with this color
	    reconstructed(x, y, 0) = palette[nearest].getR();
	    reconstructed(x, y, 1) = palette[nearest].getG();
	    reconstructed(x, y, 2) = palette[nearest].getB();
	}
    }

    delete[] palette;

    // Return the result
    return reconstructed;
}


/** Return a dithered image, from the paletteImage colors and using
    the thresholdImage threshold */
CImg<unsigned char> dither(CImg<unsigned char> const& image, CImg<unsigned char> const& paletteImage, CImg<unsigned char> const& thresholdImage) {
    // The resulting image, reconstructed from the original with the palette's colors
    CImg<unsigned char> dithered(image.width(), image.height(), 1, 3, 0);

    // Temporary color variable
    Color c;
    // The nearest color index
    int nearest;
    // Distance of the nearest color
    long nearestDist;
    // Distance between two colors
    long dist;

    // The palette colors array
    Color * palette;
    // The number of colors in the palette
    int colorCount;
    
    // The threshold map (values between 0 and 1)
    float * threshold;
   // The width and height of the threshold map
    int threshW;
    int threshH;
    
    threshold = new float[threshW*threshH];;
    // The width and height of the threshold map
    threshW = thresholdImage.width();
    threshH = thresholdImage.height();



    // Grab the colors from the palette and set the colorCount
    palette = parsePalette(paletteImage, colorCount);

    // For each pixel in the image, find the best color
    for (int y = 0 ; y < image.height() ; y++) {
	for (int x = 0 ; x < image.width() ; x++) {
	    // The color to approximate
	    c = getColor(x, y, image);
	    // The current nearest color is the first...
	    nearest = 0;
	    nearestDist = 16777216; // (256*256*256) = maximum value
				    // of distance2(color1, color2)
	    // Find the closest color in the palette
	    for (int k = 0 ; k < colorCount ; k++ ) {
		dist = Color::distance2(c, palette[k]);
		if (dist < nearestDist) {
		    nearestDist = dist;
		    nearest = k;
		}
	    }
	    // Set the target pixel with this color
	    dithered(x, y, 0) = palette[nearest].getR();
	    dithered(x, y, 1) = palette[nearest].getG();
	    dithered(x, y, 2) = palette[nearest].getB();
	}
    }

    // Return the result
    return dithered;
}


float bayerFunction(int p) {
    int q = p ^ (p >> 3);
    long result = (((p & 4) >> 2) | ((q & 4) >> 1)
		   | ((p & 2) << 1) | ((q & 2) << 2)
		   | ((p & 1) << 4) | ((q & 1) << 5));
    return (float) result / 64;
}

int main(int argc, char* argv[]) {

    // The image file path
    char * fileImage;
    // The palette image file path
    char * filePalette;
    // The threshold pattern image file path
    char * fileThreshold;

    // If not enough arguments were given when called
    if (argc != 4) {
	// Display an error and exit
	cerr << "Expected : " << argv[0] << " %imageFile %paletteFile %thresholdFile" << endl;
	exit(1);
    }
  
    fileImage = argv[1];
    filePalette = argv[2];
    fileThreshold = argv[3];

    // The image
    CImg<unsigned char>  image(fileImage);
    // The palette image
    CImg<unsigned char>  paletteImage(filePalette);
    // The threshold image
    CImg<unsigned char>  thresholdImage(fileThreshold);

    // The dithered result image
    CImg<unsigned char>  result(fileThreshold);

    result = nodither(image, paletteImage);

    // Display the result
    CImgDisplay result_disp(result, "Result");    

    cout << "Enter to finish" << endl;
    cin.ignore();

    /*
    // Save dithered image
    if(argc >= 5) {
	result.save(outputFile);
    } else {
	// Create a generic file name
	string string_output = std::string(filePalette) + "-" 
	    + std::string(fileThreshold) + "-dithered-" + std::string(fileImage);
	const char * char_output = string_palette.c_str();
	result.save(char_output);
	cout << endl << "Palette image saved under " << char_output << endl;
    }
    */

    return 0;
}
