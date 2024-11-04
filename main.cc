/**
 * @author Garrett Rhoads
 * @date 10/11/24
 * @brief Ascii Art generator, png to ascii
 */

#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>

extern "C" {
    #define STB_IMAGE_IMPLEMENTATION
    #include "stb_image.h"
}

using namespace std;

#ifndef IMAGE_H
#define IMAGE_H

class Image {
public:
// Public methods
    Image();
    Image(string filename);
    ~Image();

    void to_ascii(const int& scalar);
    bool load();
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
    vector<vector<int> > _greyscale_image;
    vector<vector<int> > _dog;
    int _width;
    int _height;
    int _scaled_width;
    int _scaled_height;
    int _scalar;
    string _filename;
    const string _ascii_luminance = " .:cCoPO0@";
};

#endif

/**
 * @brief Construct a new Image object
 * 
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
void Image::to_ascii(const int& scalar) {
    _scalar = scalar;

    _scaled_width = _width / _scalar;
    _scaled_height = _height / _scalar;

    string output_filename = "output.txt";
    ofstream out(output_filename);

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
                    out << "--";
                } else if (theta < 0.4) {
                    out << back_slash << back_slash;
                } else if (theta < 0.6) {
                    out << "||";
                } else {
                    out << "//";
                }
            } else {
                avg_lumin = _greyscale_image[i][j] * 100;
                ascii_idx = (avg_lumin / (25500 / 
                (_ascii_luminance.length() - 1)));
                
                out << _ascii_luminance[ascii_idx] << _ascii_luminance[ascii_idx];
            }
        }
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

    img.to_ascii(scalar);

    return 0;
}