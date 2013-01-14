#include <map>
#include <set>
#include <string>
#include <vector>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sstream>
#include <inttypes.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bmp.h"

using namespace std;
class ImageFile {
public:
    int m_number;
    off_t m_filesize;
	string m_filename;
    bool operator < (const ImageFile& A) const {
        return this->m_number < A.m_number;
    }
};

class ImageBlock {
public:
	int m_x;//in pixels
	int m_y;//in pixels
	int m_frame_num;//frame number
	uint8_t m_pixels[192];//pixels
};

namespace {
	const int SUCCESS = 0;
	// error codes
	const int ERROR_WITH_INPUT = -1;
	const int ERROR_WITH_FORK = -2;
	const int ERROR_WITH_OUTPUT_DIR = -3;
	const int ERROR_WITH_FFMPEG = -4;
	const int ERROR_WITH_CJPEG = -5;
	// global variables
	int fd;
	const int WIDTH = 7680;
	const int HEIGHT = 4800;
	const int BLOCKS_PER_IMAGE = (WIDTH/8) * (HEIGHT/8);
	const int FILENAME_BUF_SIZE = 1024;
	const char output_dirname[] = "encoder_output";
	std::map<std::string,std::pair<int,int> > sizes;
	std::set<ImageFile> images;
	std::vector<ImageBlock> blocks;
} // end of namespace

void fill_sizes(){
	sizes["sqcif"]	= std::make_pair(128,96);
	sizes["qcif"]	= std::make_pair(176,144);
	sizes["cif"]	= std::make_pair(352,288);
	sizes["4cif"]	= std::make_pair(744,576);
	sizes["16cif"]	= std::make_pair(1408,1152);
	sizes["qqvga"]	= std::make_pair(160,120);
	sizes["qvga"]	= std::make_pair(320,240);
	sizes["vga"]	= std::make_pair(640,480);
	sizes["svga"]	= std::make_pair(800,600);
	sizes["xga"]	= std::make_pair(1024,768);
	sizes["uxga"]	= std::make_pair(1600,1200);
	sizes["qxga"]	= std::make_pair(2048,1536);
	sizes["sxga"]	= std::make_pair(1280,1024);
	sizes["qsxga"]	= std::make_pair(2560,2048);
	sizes["hsxga"]	= std::make_pair(5120,4096);
	sizes["wxga"] 	= std::make_pair(1366,768);
	sizes["wsxga"]	= std::make_pair(1600,1024);
	sizes["wuxga"]	= std::make_pair(1920,1200);
	sizes["woxga"] 	= std::make_pair(2560,1600);
	sizes["wqsxga"]	= std::make_pair(3200,2048);
	sizes["wquxga"]	= std::make_pair(3840,2400);
	sizes["whsxga"]	= std::make_pair(6400,4096);
	sizes["whuxga"]	= std::make_pair(7680,4800);
	sizes["cga"]	= std::make_pair(320,200);
}

bool compare_two_blocks(const uint8_t* a, const uint8_t* b, int stride){
	for(int i=0;i<8;i++){
		for(int j=0;j<8;j++){
			for(int k=0;k<3;k++)
				if(a++ != b++)
					return false;
		}
		a -= 24;
		a += stride*3;
		b -= 24;
		b += stride*3;
	}
	return true;
}

int main(int argc, char **argv) {
    if(argc < 4 || strcmp(argv[1],"-s")){
	    cerr << "Usage: " << argv[0] << "-s size full_pathname_of_video_file_to_encode" << endl;
	    return ERROR_WITH_INPUT;
    }
    
    fill_sizes();
	
	string size_of_output_file(argv[2]), fullinfile(argv[3]);
	if(sizes.find(size_of_output_file)==sizes.end()){
		return ERROR_WITH_INPUT;
	}
    
    char infile[FILENAME_BUF_SIZE];
	size_t pos = fullinfile.rfind("/");
	strcpy(infile,fullinfile.substr(pos+1).c_str());
	
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	if((fd = open(fullinfile.c_str(), O_RDWR, mode))<0){
		cerr << "input file: " << fullinfile << " could not be opened with error: " << strerror(errno) << endl;
		return ERROR_WITH_INPUT;
	}
	
	struct stat fStat;
	if(fstat(fd,&fStat)<0){
		cerr << "fstat of input file: " << fullinfile << " failed with error: " << strerror(errno) << endl;
		return ERROR_WITH_INPUT;
	}
	
	if(!fStat.st_size){
		cerr << "invalid input file: " << fullinfile << endl;
		return ERROR_WITH_INPUT;
	}
	
	char current_dir[FILENAME_BUF_SIZE];
	char output_dir[2*FILENAME_BUF_SIZE];
	char filepath[2*FILENAME_BUF_SIZE];
	
	if(!getcwd(current_dir,FILENAME_BUF_SIZE)){
		cerr << "cwd error: " << strerror(errno) << endl;
		return ERROR_WITH_OUTPUT_DIR;
	}
	
	strcpy(output_dir,current_dir);
	strcat(output_dir,"/");
	strcat(output_dir,output_dirname);

	DIR* dir;
	struct dirent *next_file;
	if(!(dir=opendir(output_dir))){
		if(errno==ENOENT){
			if (mkdir(output_dir, mode) != 0 && errno != EEXIST){
				cerr << "could not create the output directory: " << output_dir << " with error: " << strerror(errno) << endl;
				return ERROR_WITH_OUTPUT_DIR;
			}
		}
		dir = opendir(output_dir);
	}
	
	while (next_file = readdir(dir)){
        	snprintf(filepath,FILENAME_BUF_SIZE,"%s/%s",output_dir,next_file->d_name);
        	remove(filepath);
    }
	
	closedir(dir);
	
	if(chdir(output_dir)<0){
		cerr << "could not change the working directory to: " << output_dir << " with error: " << strerror(errno) << endl;
		return ERROR_WITH_OUTPUT_DIR;
	}
	
	int tfd;
	if((tfd = open(infile, O_RDWR|O_CREAT, mode))<0){
		cerr << "file: " << infile << " could not be opened with error: " << strerror(errno) << endl;
		return ERROR_WITH_INPUT;
	}
	
	char* temp = new char[fStat.st_size];
	read(fd,temp,fStat.st_size);
	close(fd);
	write(tfd,temp,fStat.st_size);
	close(tfd);
	delete [] temp;
	
	pid_t ffmpeg;
	if((ffmpeg = fork())<0){
		cerr << "failed to fork with error: " << strerror(errno) << endl;
		return ERROR_WITH_FORK;
	} else if(ffmpeg==0){
		if(execlp("ffmpeg","ffmpeg","-i",infile,"-v","verbose","-s",size_of_output_file.c_str(),"file%d.bmp")<0){
			cerr << "ffmpeg failed with error: " << strerror(errno) << endl;
			return ERROR_WITH_FFMPEG;
		}	
	} else {
		int status;
		if(wait(&status)<0){
			if(errno != ECHILD){
				cerr << "wait failed with error: " << strerror(errno) << endl;
				return ERROR_WITH_FORK;
			}
		}
		if(status<0){
			cerr << "ffmpeg failed with error: " << status << endl;
			return ERROR_WITH_FFMPEG;
		}
		cout << "Output from ffmpeg: " << status << endl;
	}
	
	if(!(dir=opendir(output_dir))){
		cerr << "could not find the output directory: " << output_dir << " with error: " << strerror(errno) << endl;
		return ERROR_WITH_OUTPUT_DIR;
	}
	
	uint32_t total_input_size = 0, final_input_size = 0;
	while (next_file = readdir(dir)){
		const char* tmp;
		if(!(tmp = strstr(next_file->d_name,".bmp")))
			continue;
		string outfilename = next_file->d_name;
		int zfd;
		if((zfd = open(outfilename.c_str(), O_RDONLY, mode))<0){
			cerr << "bmp file: " << outfilename << " could not be opened with error: " << strerror(errno) << endl;
			return ERROR_WITH_INPUT;
		}
		struct stat gStat;
		if(fstat(zfd,&gStat)<0){
			cerr << "fstat of bmp file: " << outfilename << " failed with error: " << strerror(errno) << endl;
			return ERROR_WITH_INPUT;
		}
		total_input_size += gStat.st_size;
        ImageFile jf;
		jf.m_number = atoi(outfilename.c_str()+strlen("file"));
		jf.m_filename = outfilename;
		jf.m_filesize = gStat.st_size;
		images.insert(jf);
		close(zfd);
    }
    	
    closedir(dir);
	
	uint32_t w,h;
	stringstream ss;
	int z=0,frame = 0; 
	uint8_t* xdata = NULL, *data = NULL;
	for(set<ImageFile>::iterator iter = images.begin(); iter != images.end(); ++iter, frame++){
		bitmap_image bmi(iter->m_filename);
		bmi.load();
		if(iter==images.begin()){
			w = bmi.get_width();
			h = bmi.get_height();
		}
		xdata = bmi.get_data();
		if(data){
			uint8_t* d = data, *t = xdata;
			//compare these two images//
			for(int i=0;i<h;i+=8){
				for(int j=0;j<w;j+=8){
					if(!compare_two_blocks(d,t,w)){
							ImageBlock ib;
							ib.m_frame_num = frame;
							ib.m_x = j;
							ib.m_y = i;
							for(int k=0;k<8;k++){
								memcpy(ib.m_pixels+k*24,t+w*3,24);
							}
							blocks.push_back(ib);
							if(blocks.size()==BLOCKS_PER_IMAGE){
								/* save the output image */
								uint8_t* image = new uint8_t[WIDTH*HEIGHT*3];
								const int BLOCKS_PER_STRIDE = WIDTH/8;
								for(int a=0;a<BLOCKS_PER_IMAGE;a++){
									uint8_t* x = image + (a/BLOCKS_PER_STRIDE)*24*WIDTH + (a%BLOCKS_PER_STRIDE)*24;
									uint8_t* y = blocks[a].m_pixels;
									for(int b=0;b<8;b++){
										memcpy(x,y,24);
										x += WIDTH*3;
										y += 24;
									}
								}
								ss.str("");
								ss << "blitz" << z++ << ".bmp";
								bitmap_image bmp(ss.str());
								bmp.set_height(HEIGHT);
								bmp.set_width(WIDTH);
								bmp.set_data(image);
								bmp.save();
								blocks.clear();
							}
					}
					d += 24;
					t += 24;
				}
				d += 21*w;
				t += 21*w;
			}
		} else {
			data = new uint8_t[w*h*3];
			ss.str("");
			ss << "blitz" << z++ << ".bmp";
			rename(iter->m_filename.c_str(),ss.str().c_str());
		}
		memcpy(data,xdata,w*h*3);
		remove(iter->m_filename.c_str());
	}
    	
    /*save the last image*/
	uint8_t* image = new uint8_t[WIDTH*HEIGHT*3];
	const int BLOCKS_PER_STRIDE = WIDTH/8;
	for(int a=0;a<blocks.size();a++){
		uint8_t* x = image + (a/BLOCKS_PER_STRIDE)*24*WIDTH + (a%BLOCKS_PER_STRIDE)*24;
		uint8_t* y = blocks[a].m_pixels;
		for(int b=0;b<8;b++){
			memcpy(x,y,24);
			x += WIDTH*3;
			y += 24;
		}
	}
	ss.str("");
	ss << "blitz" << z++ << ".bmp";
	bitmap_image bmp(ss.str());
	bmp.set_height(HEIGHT);
	bmp.set_width(WIDTH);
	bmp.set_data(image);
	bmp.save();
	blocks.clear();
	
	/* convert to jpeg */
	if(!(dir=opendir(output_dir))){
		cerr << "could not find the output directory: " << output_dir << " with error: " << strerror(errno) << endl;
		return ERROR_WITH_OUTPUT_DIR;
	}
	
	while (next_file = readdir(dir)){
		const char* tmp;
		if(!(tmp = strstr(next_file->d_name,".bmp")))
			continue;
		pid_t cjpeg;
		string outfilename = next_file->d_name;
		outfilename = outfilename.substr(0,outfilename.find_last_of('.')) + ".jpg";
		if((cjpeg = fork())<0){
			cerr << "failed to fork with error: " << strerror(errno) << endl;
			return ERROR_WITH_FORK;
		} else if(cjpeg==0){
			if(execlp("cjpeg","cjpeg","-sample","1x1","-verbose","-outfile",outfilename.c_str(),next_file->d_name)<0){
				cerr << "cjpeg failed with error: " << strerror(errno) << endl;
				return ERROR_WITH_CJPEG;
			}
		} else {
			int status;
			if(wait(&status)<0){
				if(errno != ECHILD){
					cerr << "wait failed with error: " << strerror(errno) << endl;
					return ERROR_WITH_FORK;
				}
			}
			if(status<0){
				cerr << "cjpeg failed with error: " << status << endl;
				return ERROR_WITH_CJPEG;
			}
			struct stat zstat;
			if(!(fstat(fd,&zstat)<0)){
				final_input_size += zstat.st_size;
			}
			remove(next_file->d_name);
		}
    }
    
    closedir(dir);
	return SUCCESS;
}
