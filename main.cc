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

/**
 * @brief Prints a matrix
 * 
 * @param matrix - matrix to print
 * @param width - width of matrix
 * @param height - height of matrix
 */
void print_matrix(const vector<vector<int> > & matrix, const int & width, const int & height) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int val = matrix[i][j];
            if (val < 10) {
                cout << "  " << val << " ";
            } else if (val < 100) {
                cout << " " << val << " ";
            } else {
                cout << val << " ";
            }
        }
        cout << endl;
    }
}

/**
 * @brief Loads the image into a vecotr storing rgb data
 * 
 * @param image - the array to stor info in
 * @param filename - name of image file 
 * @param x
 * @param y 
 * @return true - if the image was successfully loaded
 * @return false  - if the image wasn't successfully loaded
 */
bool load_image(vector<unsigned char>& image, const string& filename, int& x, int&y) {
    int n;
    unsigned char* data = stbi_load(filename.c_str(), &x, &y, &n, 4);
    if (data != nullptr) {
        image = vector<unsigned char>(data, data + x * y * 4);
    }
    stbi_image_free(data);
    return (data != nullptr);
}

/**
 * @brief Computes a gaussain blure and puts it in the blurred_matrix
 * 
 * @param blurred_matrix - the matrix where the blur is stored
 * @param lumin_matrix - greyscale matrix
 * @param width - width of grayscale matrix
 * @param height - height of greyscale matrix
 * @param kernel - blur kernel
 * @param kernel_sum - sum of blur kernel
 */
void gaussian_blur(vector<vector<int> > & blurred_matrix, 
                   const vector<vector<int> > & lumin_matrix, 
                   const int & width, const int & height,
                   const vector<int> & kernel, const int & kernel_sum) {

    vector<int> blurred_matrix_row;
    int kernel_size = (log2(kernel_sum) / 2) + 1;
    int blur = 0;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (!(((i - ((kernel_size / 2) - 1)) <= 0) || 
                  ((j - ((kernel_size / 2) - 1)) <= 0) || 
                  ((i + ((kernel_size / 2) - 1)) >= (height - 1)) || 
                  ((j + ((kernel_size / 2) - 1)) >= (width - 1)))) {

                for (int k = 0; k < kernel_size; k++) {
                    for (int l = 0; l < kernel_size; l++) {
                        blur += lumin_matrix[i + k - (kernel_size / 2)][j + l - (kernel_size / 2)] * (kernel[k] * kernel[l]);
                    }
                }
                blur = blur / kernel_sum;
                blurred_matrix_row.push_back(blur);
            } else {
                blurred_matrix_row.push_back(0);
            }
        }
        blurred_matrix.push_back(blurred_matrix_row);
        blurred_matrix_row.clear();
    }
}

/**
 * @brief Calculates the average luminance of a sub array based on the 
 *        downscaling factor
 * 
 * @param image - image vector with stored rgb data
 * @param width - number of pixels wide an image is
 * @param scalar - the downscaling factor
 * @param x_pos - x position of top left corner of the sub-pixel
 * @param y_pos - y position of top left corner of the sub-pixel
 * @return int - average luminance
 */
int avg_luminance(const vector<unsigned char> & image, 
                  const int & width, const int & scalar, 
                  const int & x_pos, const int & y_pos) {

    const size_t RGBA = 4;
    int r, g, b;
    int avg_lumin = 0;

    for (int i = (y_pos * scalar); i < (scalar * (y_pos + 1)); i++) {
        for (int j = (x_pos * scalar); j < (scalar * (x_pos + 1)); j++) {
            size_t index = RGBA * (i * width + j);
            r = static_cast<int>(image[index + 0]);
            g = static_cast<int>(image[index + 1]);
            b = static_cast<int>(image[index + 2]);

            avg_lumin += (r + g + b) / 3;
        }
    }
    avg_lumin = avg_lumin / (scalar * scalar);
    return avg_lumin;
}

/**
 * @brief Converts the original image to the scaled down grayscaled image
 * 
 * @param lumin_matrix - finished scaled grayscale matrix
 * @param image - original image data
 * @param width - width of original image
 * @param height - height of original image
 * @param end_width - sets this to the final width of the image
 * @param end_height - sets this to the final height of the image
 * @param scalar - the downscaling factor of the image
 */
void scaled_lumin_matrix(vector<vector<int> > & lumin_matrix, 
                         const vector<unsigned char> & image, 
                         const int & width, const int & height,
                         int & end_width, int & end_height,  
                         const int & scalar) {

    end_width = width / scalar;
    end_height = height / scalar;
    int avg_lumin = 0;
    vector<int> lumin_matrix_row;

    for (int i = 0; i < end_height; i++) {
        for (int j = 0; j < end_width; j++) {
            avg_lumin = avg_luminance(image, width, scalar, j, i);
            lumin_matrix_row.push_back(avg_lumin);
        }
        lumin_matrix.push_back(lumin_matrix_row);
        lumin_matrix_row.clear();
    }
}

/**
 * @brief Detects if if the pixel at x_pos, y_pos is a part of an edge using 
 *        the sobel edge detection algorithm
 * 
 * @param lumin_matrix - scaled brightness image
 * @param x_pos - x position of the pixel we want to check
 * @param y_pos - y position of the pixel we want to check
 * @param theta - angle of the edge if detected [0, 1]
 * @return true - edge was detected
 * @return false - edge wasn't detected
 */
bool edge_dector(const vector<vector<int> > & lumin_matrix, 
                 const int & x_pos, const int & y_pos,
                 double & theta) {

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

    double Gx = 0.0001;
    double Gy = 0.0;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            Gx += lumin_matrix[i + y_pos - 1][j + x_pos - 1] * x_kernel[i][j];
            Gy += lumin_matrix[i + y_pos - 1][j + x_pos - 1] * y_kernel[i][j];
        }
    }
    theta = 0.0;
    if (sqrt((Gx * Gx) + (Gy * Gy)) > 400) {
        theta = (atan(Gy / Gx) / M_PI) + 0.5;
        return true;
    }
    return false;
}

/**
 * @brief Converts the average luminance of each pixel, or sub-pixel, to an 
 *        ascii value 
 * 
 * @param image - image data vector
 * @param width - original width of image
 * @param height - original height of image
 * @param scalar - downscaling factor
 * @param ascii_luminance - encodes the brightness of every ascii character
 */
void image_to_ascii(const vector<unsigned char> & image, 
                    const int & width, const int & height, 
                    const int & scalar, const string & ascii_luminance) {
    
    string output_filename = "output.txt";
    ofstream out(output_filename);
    
    vector<vector<int> > lumin_matrix;
    int end_width;
    int end_height;

    scaled_lumin_matrix(lumin_matrix, image, width, height, end_width, end_height, scalar);

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

    vector<vector<int> > gause_1;
    vector<vector<int> > gause_2;
    gaussian_blur(gause_1, lumin_matrix, end_width, end_height, kernel_1, 16);
    gaussian_blur(gause_2, lumin_matrix, end_width, end_height, kernel_2, 4096);
    vector<vector<int> > dog_matrix;
    vector<int> dog_row;

    for (int i = 0; i < end_height; i++) {
        for (int j = 0; j < end_width; j++) {
            if ((gause_1[i][j] - gause_2[i][j]) > 9) {
                dog_row.push_back(255);
            } else {
                dog_row.push_back(0);
            }
        }
        dog_matrix.push_back(dog_row);
        dog_row.clear();
    }

    int avg_lumin;
    int ascii_idx;
    double theta;

    for (int i = 0; i < end_height; i++) {
        for (int j = 0; j < end_width; j++) {
            if ((!((i == 0) || (j == 0) || (i == (end_height - 1)) || (j == (end_width - 1)))) && 
                (edge_dector(dog_matrix, j, i, theta)) && (lumin_matrix[i][j] < 96)) {

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
                avg_lumin = lumin_matrix[i][j] * 100;
                ascii_idx = (avg_lumin / (25500 / (ascii_luminance.length() - 1)));
                
                out << ascii_luminance[ascii_idx] << ascii_luminance[ascii_idx];
            }
        }
        out << endl;
    }
    out.close();
}

/**
 * @brief I/O and controls operation of the program
 * 
 * @return int 
 */
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
    
    string ascii_luminance = " .:cCoPO0@";

    image_to_ascii(image, width, height, scalar, ascii_luminance);

    return 0;
}