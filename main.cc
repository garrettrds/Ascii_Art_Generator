/**
 * @author Garrett Rhoads
 * @date 10/11/24
 * @brief Ascii Art generator, png to ascii
 */

#include <iostream>
#include <vector>
#include <fstream>

extern "C" {
    #define STB_IMAGE_IMPLEMENTATION
    #include "stb_image.h"
}

using namespace std;

bool load_image(vector<unsigned char>& image, const string& filename, int& x, int&y) {
    int n;
    unsigned char* data = stbi_load(filename.c_str(), &x, &y, &n, 4);
    if (data != nullptr) {
        image = vector<unsigned char>(data, data + x * y * 4);
    }
    stbi_image_free(data);
    return (data != nullptr);
}

int avg_lumenance(const vector<unsigned char> & image, const int & width, const int & scalar, const int & x_pos, const int & y_pos) {

    const size_t RGBA = 4;
    int r, g, b;
    int avg_lumen = 0;

    for (int i = (y_pos * scalar); i < (scalar * (y_pos + 1)); i++) {
        for (int j = (x_pos * scalar); j < (scalar * (x_pos + 1)); j++) {
            size_t index = RGBA * (i * width + j);
            r = static_cast<int>(image[index + 0]);
            g = static_cast<int>(image[index + 1]);
            b = static_cast<int>(image[index + 2]);

            avg_lumen += (r + g + b) / 3;
            //cout << avg_lumen << " ";
            //cout << "x_pos = " << i << endl << "y_pos = " << j << endl;
        }
    }
    avg_lumen = avg_lumen / (scalar * scalar);
    return avg_lumen;
}

void old_image_to_ascii(const vector<unsigned char> & image, 
                        const int & width, 
                        const int & height, 
                        const string & ascii_lumenance) {

    string output_filename = "output.txt";
    ofstream out(output_filename);

    const size_t RGBA = 4;
    int r, g, b;
    int avg_lumen;
    int ascii_idx;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            size_t index = RGBA * (i * width + j);
            r = static_cast<int>(image[index + 0]);
            g = static_cast<int>(image[index + 1]);
            b = static_cast<int>(image[index + 2]);

            avg_lumen = ((r + g + b) / 3) * 100;
            ascii_idx = (avg_lumen / (25500 / (ascii_lumenance.length() - 1)));
            
            out << ascii_lumenance[ascii_idx] << ascii_lumenance[ascii_idx];
        }
        out << endl;
    }
    out.close();
}

void image_to_ascii(const vector<unsigned char> & image, 
                    const int & width,
                    const int & height, 
                    const int & scalar, 
                    const string & ascii_lumenance) {

    string output_filename = "output.txt";
    ofstream out(output_filename);

    int end_width = width / scalar;
    int end_height = height / scalar;

    int ascii_idx;
    int avg_lumen;

    for (int i = 0; i < end_height; i++) {
        for (int j = 0; j < end_width; j++) {
            avg_lumen = avg_lumenance(image, width, scalar, j, i) * 100;
            //cout << avg_lumen << " ";
            ascii_idx = (avg_lumen / (25500 / (ascii_lumenance.length() - 1)));
            
            out << ascii_lumenance[ascii_idx] << ascii_lumenance[ascii_idx];
            //cout << j << " ";
        }
        //cout << endl;
        out << endl;
    }
    out.close();
}


int main() {
    string img_filename;
    int scalar;
    cout << "File name:" << endl;
    cin >> img_filename;
    
    int width, height;
    vector<unsigned char> image;
    bool success = load_image(image, img_filename, width, height);
    if (!success) {
        cout << "Error loading image\n";
        return 1;
    }

    cout << "Input image dimensions:" << endl << width << " x " << height << endl;
    cout << "Image downscaling factor:" << endl;
    cin >> scalar;
    
    string ascii_lumenance = " `.-':_,^=;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@";
    
    image_to_ascii(image, width, height, scalar, ascii_lumenance);

    return 0;
}