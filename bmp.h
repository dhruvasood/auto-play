#ifndef BMP_H
#define BMP_H

#include <string>
#include <fstream>
#include <iostream>
#include <inttypes.h>

class bitmap_image
{
public:
	bitmap_image(const std::string& filename);
	~bitmap_image();
	void load();
	void save();
	uint32_t get_height();
	uint32_t get_width();
	uint8_t* get_data();
	void set_height(uint32_t val);
	void set_width(uint32_t val);
	void set_data(uint8_t* data);
private:
	std::string file_name_;
	uint8_t* data_;
	uint32_t bytes_per_pixel_;
	uint32_t length_;
	uint32_t width_;
	uint32_t height_;
	uint32_t row_increment_;

	struct bitmap_file_header 
	{
		uint16_t type;
		uint32_t size;
		uint16_t reserved1;
		uint16_t reserved2;
		uint32_t off_bits;

		uint32_t struct_size(){
			return sizeof(type) + sizeof(size) + sizeof(reserved1) +sizeof(reserved2) + sizeof(off_bits);
		}
   };

   struct bitmap_information_header
   {
	   uint32_t size;
	   uint32_t width;
	   uint32_t height;
	   uint16_t planes;
	   uint16_t bit_count;
	   uint32_t compression;
	   uint32_t size_image;
	   uint32_t x_pels_per_meter;
	   uint32_t y_pels_per_meter;
	   uint32_t clr_used;
	   uint32_t clr_important;
	   
	   uint32_t struct_size(){
         return sizeof(size) + sizeof(width) + sizeof(height) + sizeof(planes) + sizeof(bit_count) + sizeof(compression)  +
                sizeof(size_image) + sizeof(x_pels_per_meter) + sizeof(y_pels_per_meter) + sizeof(clr_used) + sizeof(clr_important);   
	   }
   };
   
   template<typename T>
   void read_from_stream(std::ifstream& stream, T& t);
   
   void read_bfh(std::ifstream& stream, bitmap_file_header& bfh);
   
   void read_bih(std::ifstream& stream,bitmap_information_header& bih);
   
   void create_bitmap();
   
   uint8_t* row(uint32_t row_index) const;
   
   void write_bih(std::ofstream& stream, const bitmap_information_header& bih);
   
   void write_bfh(std::ofstream& stream, const bitmap_file_header& bfh);
   
   template<typename T>
   void write_to_stream(std::ofstream& stream,const T& t);
};
   
#endif