/**
 * @author Garrett Rhoads
 * @date 10/11/24
 * @brief Ascii Art generator, png to ascii
 */

#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <chrono>
#include <thread>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;
using namespace std::this_thread; // sleep_for, sleep_until
using namespace std::chrono; // nanoseconds, system_clock, seconds

const int CHANNELS = 3;

#ifndef IMAGE_H
#define IMAGE_H

class Image {
public:
// Public methods
    Image();
    Image(string filename);
    ~Image();

    void to_ascii_index(const int& scalar);
    void to_ascii_png();
    bool load();
    bool load_palette();
    int get_width() const;
    int get_height() const;
private:
// Private methods
    void scaled_greyscale_image();
    int convolve(const int& x_pos, const int& y_pos) const;
    int convolve(const vector<vector<int> >& matrix, const int& x_pos, 
                 const int& y_pos, const vector<int>& kernel);
    
    double convolve(const int& x_pos, const int& y_pos, 
                    const vector<vector<int> >& kernel);

    bool sobel(const int& x_pos, const int& y_pos, double& theta);
    void gaussian_blur(vector<vector<int> >& blurred_image, 
                       const vector<int>& kernel);
    void dog(); // woof
// Attributes
    vector<unsigned char> _image;
    unsigned char *_output;
    vector<vector<int> > _palette;
    vector<vector<int> > _greyscale_image;
    vector<vector<int> > _dog;
    vector<vector<int> > _ascii_indeces;
    int _width;
    int _height;
    int _palette_width;
    int _palette_height;
    int _scaled_width;
    int _scaled_height;
    int _scalar;
    string _filename;
    const string _ascii_luminance = " .:cCoPO0@";
};

#endif

/**
 * @brief Construct a new Image object
 */
Image::Image() {}

/**
 * @brief Construct a new Image object
 * 
 * @param _filename - name of image file
 */
Image::Image(string filename) {
    _filename = filename;
}

/**
 * @brief Destroy the Image object
 */
Image::~Image() {}

/**
 * @brief Converts _image to ascii and exports it to output.txt
 * 
 * @param scalar - how much to down scale the image
 */
void Image::to_ascii_index(const int& scalar) {
    _scalar = scalar;

    _scaled_width = _width / _scalar;
    _scaled_height = _height / _scalar;

    string output_filename = "output.txt";
    ofstream out(output_filename);

    vector<int> _ascii_indeces_row;

    int avg_lumin;
    int ascii_idx;
    double theta;

    scaled_greyscale_image();

    dog(); // woof
    
    for (int i = 0; i < _scaled_height; i++) {
        for (int j = 0; j < _scaled_width; j++) {
            if ((!((i == 0) || (j == 0) || (i == (_scaled_height - 1)) || 
                   (j == (_scaled_width - 1)))) && (sobel(j, i, theta)) &&
                   (_greyscale_image[i][j] < 96)) {
                
                char back_slash = 92;
                
                if ((theta < 0.1) || (theta > 0.9)) {
                    _ascii_indeces_row.push_back(10);
                    out << "--";
                } else if (theta < 0.4) {
                    _ascii_indeces_row.push_back(11);
                    out << back_slash << back_slash;
                } else if (theta < 0.6) {
                    _ascii_indeces_row.push_back(12);
                    out << "||";
                } else {
                    _ascii_indeces_row.push_back(13);
                    out << "//";
                }
            } else {
                avg_lumin = _greyscale_image[i][j] * 100;
                ascii_idx = (avg_lumin / (25500 / 
                (_ascii_luminance.length() - 1)));
                _ascii_indeces_row.push_back(ascii_idx);
                out << _ascii_luminance[ascii_idx] << _ascii_luminance[ascii_idx];
            }
        }
        _ascii_indeces.push_back(_ascii_indeces_row);
        _ascii_indeces_row.clear();
        out << endl;
    }
    out.close();
}

/**
 * @brief Gets _width attribute
 * 
 * @return int 
 */
int Image::get_width() const {
    return _width;
}

/**
 * @brief Gets height attribute
 * 
 * @return int 
 */
int Image::get_height() const {
    return _height;
}

/**
 * @brief Loads the needed info into the _image attribute
 * 
 * @return true 
 * @return false 
 */
bool Image::load() {
    int n;
    unsigned char* data = stbi_load(_filename.c_str(),&_width,&_height,&n, 4);
    if (data != nullptr) {
        _image = vector<unsigned char>(data, data + _width * _height * 4);
    }
    stbi_image_free(data);
    return (data != nullptr);
}

bool Image::load_palette() {
    vector<unsigned char> _palette_1D;
    vector<int> palette_row;
    size_t RGBA = 4;
    string palette_file = "palette.png";

    int n;
    unsigned char* data = stbi_load(palette_file.c_str(),&_palette_width,&_palette_height,&n, 4);
    if (data != nullptr) {
        _palette_1D = vector<unsigned char>(data, data + _palette_width * _palette_height * 4);
    }
    stbi_image_free(data);
    if (data == nullptr) {
        return false;
    }
    int r, g, b;
    for (int i = 0; i < _palette_height; i++) {
        for (int j = 0; j < _palette_width; j++) {
            size_t index = RGBA * (i * _palette_width + j);
            r = static_cast<int>(_palette_1D[index + 0]);
            g = static_cast<int>(_palette_1D[index + 1]);
            b = static_cast<int>(_palette_1D[index + 2]);

            int avg = (r + g + b) / (RGBA - 1);
            // cout << r << " " << g << " " << b << "  ";
            palette_row.push_back(avg);
        }
        _palette.push_back(palette_row);
        palette_row.clear();
    }

    return true;
}

/**
 * @brief Finds the average luminance of an area of the iamge based on the scalar
 * 
 * @param x_pos - x position of the pixel 
 * @param y_pos - y position of the pixel
 * @return int - average luminance
 */
int Image::convolve(const int& x_pos, const int& y_pos) const {
    const size_t RGBA = 4;
    int r, g, b;
    int avg_lumin = 0;

    for (int i = (y_pos * _scalar); i < (_scalar * (y_pos + 1)); i++) {
        for (int j = (x_pos * _scalar); j < (_scalar * (x_pos + 1)); j++) {
            size_t index = RGBA * (i * _width + j);
            r = static_cast<int>(_image[index + 0]);
            g = static_cast<int>(_image[index + 1]);
            b = static_cast<int>(_image[index + 2]);

            avg_lumin += (r + g + b) / 3;
        }
    }
    avg_lumin = avg_lumin / (_scalar * _scalar);
    return avg_lumin;
}

/**
 * @brief Scales the image and greyscales it
 */
void Image::scaled_greyscale_image() {
    vector<int> greyscale_row;

    for (int i = 0; i < _scaled_height; i++) {
        for (int j = 0; j < _scaled_width; j++) {  
            greyscale_row.push_back(convolve(j, i));
        }
        _greyscale_image.push_back(greyscale_row);
        greyscale_row.clear();
    }
}

/**
 * @brief Convolves the kernel with the _greyscale_image at pixel x_pos y_pos
 * 
 * @param x_pos - x position of the pixel being convolved
 * @param y_pos - y position of the pixel being convolved
 * @param kernel - kernel to do the convolution
 * @return double - sum total after convolution
 */
double Image::convolve(const int& x_pos, const int& y_pos, 
                       const vector<vector<int> >& kernel) {
    int kernel_size = kernel.size();
    double total = 0.0;
    for (int i = 0; i < kernel_size; i++) {
        for (int j = 0; j < kernel_size; j++) {
            total += _greyscale_image[y_pos + i - (kernel_size / 2)][x_pos + j - 
            (kernel_size / 2)] * kernel[i][j];
        }
    }
    return (total);
}

/**
 * @brief Convolves matrix with kernel at x_pos y_pos of matrix
 * 
 * @param matrix - 2d array
 * @param x_pos - x position to convolve
 * @param y_pos - y position to convolve
 * @param kernel - kernel to convolve with
 * @return int - adjusted total
 */
int Image::convolve(const vector<vector<int> >& matrix, const int& x_pos, 
                    const int& y_pos, const vector<int>& kernel) {

    int total = 0;
    int kernel_size = kernel.size();
    int kernel_sum = pow(2, 2 * (kernel_size + 1));
    for (int i = 0; i < kernel_size; i++) {
        for (int j = 0; j < kernel_size; j++) {
            total += matrix[y_pos + i - (kernel_size / 2)][x_pos + j - 
            (kernel_size / 2)] * (kernel[i] * kernel[j]);
        }
    }
    total = total / kernel_sum;
    return (total);
}

/**
 * @brief Detects edges at x_pos y_pos and gives the angle of the edge
 * 
 * @param x_pos - x position of pixel to check
 * @param y_pos - y position of pixel to check
 * @param theta - angle of edge [0, 1]
 * @return true - if there is an edge
 * @return false - if there is not an edge
 */
bool Image::sobel(const int& x_pos, const int& y_pos, double& theta) {
    vector<vector<int> > x_kernel;
    vector<int> x_kernel_row;
    
    x_kernel_row.push_back(-1);
    x_kernel_row.push_back(0);
    x_kernel_row.push_back(1);
    x_kernel.push_back(x_kernel_row);
    x_kernel_row.clear();    
    x_kernel_row.push_back(-2);
    x_kernel_row.push_back(0);
    x_kernel_row.push_back(2);
    x_kernel.push_back(x_kernel_row);
    x_kernel_row.clear();
    x_kernel_row.push_back(-1);
    x_kernel_row.push_back(0);
    x_kernel_row.push_back(1);
    x_kernel.push_back(x_kernel_row);
    x_kernel_row.clear();

    vector<vector<int> > y_kernel;
    vector<int> y_kernel_row;

    y_kernel_row.push_back(-1);
    y_kernel_row.push_back(-2);
    y_kernel_row.push_back(-1);
    y_kernel.push_back(y_kernel_row);
    y_kernel_row.clear();
    y_kernel_row.push_back(0);
    y_kernel_row.push_back(0);
    y_kernel_row.push_back(0);
    y_kernel.push_back(y_kernel_row);
    y_kernel_row.clear();
    y_kernel_row.push_back(1);
    y_kernel_row.push_back(2);
    y_kernel_row.push_back(1);
    y_kernel.push_back(y_kernel_row);
    y_kernel_row.clear();

    double Gx = convolve(x_pos, y_pos, x_kernel) + 0.0001;
    double Gy = convolve(x_pos, y_pos, y_kernel);

    if (sqrt((Gx * Gx) + (Gy * Gy)) > 400) {
        theta = (atan(Gy / Gx) / M_PI) + 0.5;
        return true;
    }
    return false;
}

/**
 * @brief Blurs the image and puts it in blurred_image
 * 
 * @param blurred_image - storage for blurred image
 * @param kernel - how much u wanna blur?
 */
void Image::gaussian_blur(vector<vector<int> >& blurred_image, 
                          const vector<int>& kernel) {
    vector<int> blurred_image_row;
    int border = (kernel.size() / 2) - 1;
    
    int blur;

    for (int i = 0; i < _scaled_height; i++) {
        for (int j = 0; j < _scaled_width; j++) {
            if (!(((i - border) <= 0) || ((j - border) <= 0) || 
                  ((i + border) >= (_scaled_height - 1)) || 
                  ((j + border) >= (_scaled_width - 1)))) {
                
                blur = convolve(_greyscale_image, j, i, kernel);
                blurred_image_row.push_back(blur);
            } else {
                blurred_image_row.push_back(0);
            }
        }
        blurred_image.push_back(blurred_image_row);
        blurred_image_row.clear();
    }
}

/**
 * @brief Summons the Devourer of-- sorry, preforms a simple difference of 
 *        gaussians with a threshold of 9 and kernel sizes of 3 and 7
 */
void Image::dog() {
    vector<int> kernel_1;
    kernel_1.push_back(1);
    kernel_1.push_back(2);
    kernel_1.push_back(1);

    vector<int> kernel_2;
    kernel_2.push_back(1);
    kernel_2.push_back(6);
    kernel_2.push_back(15);
    kernel_2.push_back(20);
    kernel_2.push_back(15);
    kernel_2.push_back(6);
    kernel_2.push_back(1);

    vector<vector<int> > blur_1;
    vector<vector<int> > blur_2;

    gaussian_blur(blur_1, kernel_1);
    gaussian_blur(blur_2, kernel_2);

    vector<int> dog_row;
    for (int i = 0; i < _scaled_height; i++) {
        for (int j = 0; j < _scaled_width; j++) {
            if ((blur_1[i][j] - blur_2[i][j]) > 9) {
                dog_row.push_back(255);
            } else {
                dog_row.push_back(0);
            }
        }
        _dog.push_back(dog_row);
        dog_row.clear();
    }
}

void Image::to_ascii_png() {
    int output_size = (_scaled_width * _scalar) * (_scaled_width * _scalar) * CHANNELS;
    _output = new unsigned char[output_size];
    unsigned char *pix = _output;
    
    for (int i = 0; i < _scaled_height; i++) {
        for (int palette_row = 0; palette_row < 8; palette_row++) {
            for (int j = 0; j < _scaled_width; j++) {

                int ascii_texture_start_col = _ascii_indeces[i][j] * 8;

                for (int palette_col = ascii_texture_start_col; palette_col < (ascii_texture_start_col + 8); palette_col++) {
                    //cout << "row, col = " << palette_row << ", " << palette_col << "  ";
                    for (int c = 0; c < 3; c++) {
                        *(pix + c) = _palette[palette_row][palette_col];
                    }
                    pix += CHANNELS;
                }
            }
        }
        //cout << endl;
    }
    stbi_write_png("test2.png", _scaled_width * 8, _scaled_height * 8, CHANNELS, _output, _scaled_width * 8 * CHANNELS);
    delete[] _output;
}

/**
 * @brief I/O and controls operation of the program
 * 
 * @return int 
 */
int main() {
    string img_filename;
    int scalar;
    cout << "File name:\n";
    cin >> img_filename;
    
    Image img(img_filename);
    bool success = img.load();
    if (!success) {
        cout << "Error loading image\n";
        return 1;
    }

    cout << "Input image dimensions:\n" << img.get_width() <<
    " x " << img.get_height() << endl;
    cin >> scalar;

    img.to_ascii_index(scalar);
    bool palette_success = img.load_palette();
    if (!palette_success) {
        cout << "FFFUUUUUCCKKKK U\n";
        return 1;
    }
    img.to_ascii_png();

    // while(true) {
    //     string img_filename_base = "ezgif-frame-0";
    //     string img_filename;
    //     int scalar = 8;
    //     for (int i = 0; i < 17; i++) {
    //         if (i < 9) {
    //             //cout <<"here" << endl;
    //             img_filename = img_filename_base + "0" + to_string(i + 1) + ".png";
    //         } else {
    //             img_filename = img_filename_base + to_string(i + 1) + ".png";
    //         }
            
    //         //cout << img_filename << endl;
    //         Image img(img_filename);
    //         bool success = img.load();
    //         if (!success) {
    //             cout << "Error loading image\n";
    //             return 1;
    //         }
    //         img.to_ascii_index(scalar);
    //         //sleep_for(nanoseconds(33333333));
    //     }
    // }
    return 0;
}