#include "bmp.h"
#include <assert.h>

bitmap_image::bitmap_image(const std::string& filename): file_name_(filename), data_(0), bytes_per_pixel_(0), length_(0), width_(0), height_(0), row_increment_(0){
}

bitmap_image::~bitmap_image(){
	if(data_){
		delete [] data_;
	}
}

template<typename T>
void bitmap_image::read_from_stream(std::ifstream& stream, T& t){
	stream.read(reinterpret_cast<char*>(&t),sizeof(T));
}

void bitmap_image::read_bfh(std::ifstream& stream, bitmap_file_header& bfh) {
	read_from_stream(stream,bfh.type);
    read_from_stream(stream,bfh.size);
    read_from_stream(stream,bfh.reserved1);
    read_from_stream(stream,bfh.reserved2);
    read_from_stream(stream,bfh.off_bits);
}

void bitmap_image::read_bih(std::ifstream& stream, bitmap_image::bitmap_information_header& bih){
	read_from_stream(stream,bih.size);
    read_from_stream(stream,bih.width);
    read_from_stream(stream,bih.height);
    read_from_stream(stream,bih.planes);
    read_from_stream(stream,bih.bit_count);
    read_from_stream(stream,bih.compression);
    read_from_stream(stream,bih.size_image);
    read_from_stream(stream,bih.x_pels_per_meter);
    read_from_stream(stream,bih.y_pels_per_meter);
    read_from_stream(stream,bih.clr_used);
    read_from_stream(stream,bih.clr_important);
}

void bitmap_image::create_bitmap()
{
	row_increment_ = width_ * bytes_per_pixel_;
	length_ = width_ * height_ * bytes_per_pixel_;   
    if (0 != data_) {
         delete[] data_;
    }
    data_ = new uint8_t[length_];
}

uint8_t* bitmap_image::row(uint32_t row_index) const {
	return data_ + (row_index * row_increment_); 
}
   
void bitmap_image::load(){
	std::ifstream stream(file_name_.c_str(),std::ios::binary);
    if (!stream) {
         std::cerr << "bitmap_image::load_bitmap() ERROR: bitmap_image - file " << file_name_ << " not found!" << std::endl;
         return;
	}
	
	bitmap_file_header bfh;
	bitmap_information_header bih;

	read_bfh(stream,bfh);
	read_bih(stream,bih);

	/* 'BM' */
	if (bfh.type != 19778) {
		stream.close();
		std::cerr << "bitmap_image::load_bitmap() ERROR: bitmap_image - Invalid type value " << bfh.type << " expected 0X42 0X4D." << std::endl;
        return;
	}
	
	if (bih.bit_count != 24) {
		stream.close();
		std::cerr << "bitmap_image::load_bitmap() ERROR: bitmap_image - Invalid bit depth " << bih.bit_count << " expected 24." << std::endl;
        return;
    }

    height_ = bih.height;
    width_  = bih.width;
	assert(!(height_%8) && !(width_%8));
	
	bytes_per_pixel_ = bih.bit_count >> 3;
    int padding = (4 - ((3 * width_) % 4)) % 4;
    char padding_data[4] = {0,0,0,0};

    create_bitmap();
	
	for (uint32_t i = 0; i < height_; ++i)
    {
		uint8_t* data_ptr = row(height_ - i - 1); // read in inverted row order
        stream.read(reinterpret_cast<char*>(data_ptr),sizeof(uint8_t) * bytes_per_pixel_ * width_);
        stream.read(padding_data,padding);
    }
    
    stream.close();
}

void bitmap_image::write_bih(std::ofstream& stream, const bitmap_information_header& bih) {
	write_to_stream(stream,bih.size);
	write_to_stream(stream,bih.width);
	write_to_stream(stream,bih.height);
	write_to_stream(stream,bih.planes);
	write_to_stream(stream,bih.bit_count);
	write_to_stream(stream,bih.compression);
	write_to_stream(stream,bih.size_image);
	write_to_stream(stream,bih.x_pels_per_meter);
	write_to_stream(stream,bih.y_pels_per_meter);
	write_to_stream(stream,bih.clr_used);
	write_to_stream(stream,bih.clr_important);
}

void bitmap_image::write_bfh(std::ofstream& stream, const bitmap_file_header& bfh) {
	write_to_stream(stream,bfh.type);
	write_to_stream(stream,bfh.size);
	write_to_stream(stream,bfh.reserved1);
	write_to_stream(stream,bfh.reserved2);
	write_to_stream(stream,bfh.off_bits);
}

template<typename T>
void bitmap_image::write_to_stream(std::ofstream& stream,const T& t)
{
	stream.write(reinterpret_cast<const char*>(&t),sizeof(T));
}

void bitmap_image::save(){
	std::ofstream stream(file_name_.c_str(),std::ios::binary);
	
	if (!stream){
         std::cout << "bitmap_image::save_image(): Error - Could not open file "  << file_name_ << " for writing!" << std::endl;
         return;
    }

    bitmap_file_header bfh;
    bitmap_information_header bih;
	
	bytes_per_pixel_ = 3;

    bih.width            = width_;
    bih.height           = height_;
    bih.bit_count        = static_cast<unsigned short>(bytes_per_pixel_ << 3);
    bih.clr_important    =  0;
    bih.clr_used         =  0;
    bih.compression      =  0;
    bih.planes           =  1;
    bih.size             = 40;
    bih.x_pels_per_meter =  0;
    bih.y_pels_per_meter =  0;
    bih.size_image       = (((bih.width * bytes_per_pixel_) + 3) & 0xFFFC) * bih.height;

    bfh.type      = 19778;
    bfh.size      = 55 + bih.size_image;
    bfh.reserved1 = 0;
    bfh.reserved2 = 0;
    bfh.off_bits  = bih.struct_size() + bfh.struct_size();

    write_bfh(stream,bfh);
    write_bih(stream,bih);

	row_increment_ = width_ * bytes_per_pixel_;
	
    int padding = (4 - ((3 * width_) % 4)) % 4;
    char padding_data[4] = {0x0,0x0,0x0,0x0};
    for (uint32_t i = 0; i < height_; ++i) {
		uint8_t* data_ptr = data_ + (row_increment_ * (height_ - i - 1));
        stream.write(reinterpret_cast<char*>(data_ptr),sizeof(uint8_t) * bytes_per_pixel_ * width_);
        stream.write(padding_data,padding);
    }
    stream.close();
}
uint32_t bitmap_image::get_height(){
	return height_;
}

uint32_t bitmap_image::get_width(){
	return width_;
}

uint8_t* bitmap_image::get_data(){
	return data_;
}

void bitmap_image::set_height(uint32_t val){
	height_ = val;
}

void bitmap_image::set_width(uint32_t val){
	width_ = val;
}

void bitmap_image::set_data(uint8_t* data){
	data_ = data;
}