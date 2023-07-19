#include <iostream>
#include <fstream>
#include <vector>
#include <jpeglib.h>
#include <png.h>

using namespace std;

// Define the image format constants
const int BMP = 1;
const int JPEG = 2;
const int PNG = 3;

// Define a struct to hold the image data
struct Image {
    int width;
    int height;
    int numChannels;
};

// Function to read a BMP image file
vector<vector<unsigned char>> read_bmp_file(const string& filename) {
    vector<vector<unsigned char>> image_matrix;

    // Open the file for reading
    FILE* file = fopen(filename.data(), "rb");
    if (file == NULL) {
        printf("Error opening file %s\n", filename.data());
        exit(EXIT_FAILURE);
    }
    

    // Read the BMP header
    char header[54];
    fread(header, sizeof(uint8_t), 54, file);

    // Get the image width and height
    int width = *(int*)&header[18];
    int height = *(int*)&header[22];

    // Allocate memory for the image data
    size_t row_size = width * 3;

    image_matrix.resize(width);
    for (int i = 0; i < height; i++) {
        image_matrix[i].resize(width);
    }

    // Read the image data
    for (int i = 0; i < height; i++) {
        // Seek to the start of the current row
        fseek(file, 54 + i * row_size, SEEK_SET);

        // Read the current row of pixels into the image matrix
        for (int j = 0; j < width; j++) {
            uint8_t pixel;
            fread(&pixel, sizeof(uint8_t), 1, file);
            image_matrix[i][j] = pixel;
        }
    }
    
    // Close the file
    fclose(file);

    return image_matrix;
}

// Function to read a PNG image file
vector<vector<unsigned char>> read_jpeg_file(const string& filename) {
    vector<vector<unsigned char>> image_matrix;
    
    FILE* fp = fopen(filename.data(), "rb");
    if (!fp) {
        cerr << "Error: Could not open file " << filename << endl;
        return image_matrix;
    }
    
    struct jpeg_decompress_struct cinfo; // JPEG decompression structure
    struct jpeg_error_mgr jerr; // Error handling structure
    
    cinfo.err = jpeg_std_error(&jerr); // Initialize the error handler
    jpeg_create_decompress(&cinfo); // Initialize the decompression object
    
    jpeg_stdio_src(&cinfo, fp); // Set the input file
    
    jpeg_read_header(&cinfo, TRUE); // Read the JPEG header
    jpeg_start_decompress(&cinfo); // Start decompression
    
    // Get image information
    int width = cinfo.output_width;
    int height = cinfo.output_height;
    int numChannels = cinfo.num_components;

    image_matrix.resize(height);
    for (int i = 0; i < height; i++) {
        image_matrix[i].resize(numChannels * width);
    }

    int rowStride = width * numChannels;
    unsigned char* row_pointer;

    // Read scanlines one by one
    while (cinfo.output_scanline < cinfo.output_height) {
        row_pointer = &image_matrix[cinfo.output_scanline][0];
        jpeg_read_scanlines(&cinfo, &row_pointer, 1);
    }
    
    jpeg_finish_decompress(&cinfo); // Finish decompression
    jpeg_destroy_decompress(&cinfo); // Destroy decompression object
    fclose(fp); // Close the JPEG file
    
    // Print image information
    cout << "Width: " << width << endl;
    cout << "Height: " << height << endl;
    cout << "Number of channels: " << numChannels << endl;
    
    return image_matrix;
}


// Function to read a PNG image file
vector<vector<unsigned char>> read_png_file(const string& filename) {
    vector<vector<unsigned char>> image_matrix;

    // Open the file for reading
    ifstream file(filename, ios::binary);
    if (!file) {
        cerr << "Error: Could not open file " << filename << endl;
        return image_matrix;
    }

    png_FILE_p fp = fopen(filename.data(), "rb");
    if (!fp) {
        cerr << "Error: Could not open file " << filename << endl;
        return image_matrix;
    }

    // Initialize the PNG structures
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr) {
        cerr << "Error: png_create_read_struct failed." << endl;
        file.close();
        return image_matrix;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        cerr << "Error: png_create_info_struct failed." << endl;
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        file.close();
        return image_matrix;
    }

    // Set up error handling
    if (setjmp(png_jmpbuf(png_ptr))) {
        cerr << "Error: PNG error during read." << endl;
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        file.close();
        return image_matrix;
    }

    // Set the input file
    png_init_io(png_ptr, fp);

    // Read the PNG header
    png_read_info(png_ptr, info_ptr);

    // Get the image width and height
    int width = png_get_image_width(png_ptr, info_ptr);
    int height = png_get_image_height(png_ptr, info_ptr);

    // Set the PNG transforms
    png_set_expand(png_ptr);
    png_set_gray_to_rgb(png_ptr);

    // Update the info pointer with the transforms
    png_read_update_info(png_ptr, info_ptr);

    // Allocate memory for the image data
    size_t row_stride = png_get_rowbytes(png_ptr, info_ptr);
    size_t image_size = row_stride * height;
    image_matrix.resize(image_size);

    // Read the image data
    png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);
    for (int y = 0; y < height; y++) {
        unsigned char* row_pointer = &image_matrix[y * row_stride][0];
        memcpy(row_pointer, row_pointers[y], row_stride);
    }

    // Clean up the PNG structures
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

    // Close the file
    fclose(fp);

    return image_matrix;
}

void write_jpeg(vector<vector<unsigned char>> image, const string& filename) {
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    FILE *outfile = fopen(filename.data(), "wb"); // pointer to the output file
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = // image width
    cinfo.image_height = // image height
    cinfo.input_components = // number of color channels
    cinfo.in_color_space = JCS_RGB; // color space
    jpeg_set_defaults(&cinfo);

    jpeg_start_compress(&cinfo, TRUE);
    unsigned char* scanline = (unsigned char*)malloc(cinfo.image_width * cinfo.input_components);
    while (cinfo.next_scanline < cinfo.image_height) {
        // copy the next scanline of the image data into the scanline buffer
        // ...
        jpeg_write_scanlines(&cinfo, &scanline, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    free(scanline);

    fclose(outfile);
}

void write_png(vector<vector<unsigned char>> image, const string& filename) {
    
}

// Function to read an image file
vector<vector<unsigned char>> imread(const string& filename) {
    vector<vector<unsigned char>> image;

    // Determine the image format based on the file extension
    int format = 0;
    string extension = filename.substr(filename.find_last_of(".") + 1);
    if (extension == "bmp") {
        format = BMP;
    } else if (extension == "jpg" || extension == "jpeg") {
        format = JPEG;
    } else if (extension == "png") {
        format = PNG;
    } else {
        cerr << "Error: Unsupported image format." << endl;
        return image;
    }

    // Call the appropriate function to read the image file
    // Call the appropriate function to read the image file
    switch (format) {
        case BMP:
            return read_bmp_file(filename);
        case JPEG:
            return read_jpeg_file(filename);
        case PNG:
            return read_png_file(filename);
    }

    return image;
}

int main()
{
    vector<vector<unsigned char>> image;
    image = imread("images/test_image_1.jpeg");

    return 0;
}

