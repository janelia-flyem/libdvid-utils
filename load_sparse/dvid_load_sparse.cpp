#include <libdvid/DVIDNodeService.h>

#include <iostream>
#include <string>
#include <fstream>

#include <vector>
#include <tr1/unordered_map>
#include <tr1/unordered_set>

using std::cout; using std::endl;
using std::ifstream;

using std::string;
using std::tr1::unordered_map;
using std::tr1::unordered_set;
using std::vector;
using std::pair;
using std::make_pair;
using std::cin;

const char * USAGE = "<prog> <dvid-server> <uuid> <label name> <sparse file> <body ID>";
const char * HELP = "Program takes a sparse volume and loads it into DVID";

int read_int(ifstream& fin)
{
    char buffer[4]; // reading signed ints
    fin.read(buffer, 4);
    return *((int*) buffer);
}

int main(int argc, char** argv)
{
    if (argc != 6) {
        cout << USAGE << endl;
        cout << HELP << endl;
        exit(1);
    }
    
    // create DVID node accessor 
    libdvid::DVIDNodeService dvid_node(argv[1], argv[2]);
    string label_name = string(argv[3]);
  
    ifstream fin(argv[4]);
   
    unsigned long long new_body_id = (unsigned long long)(atoi(argv[5])); 

    // read sparse volume one at a time
    int num_stripes = read_int(fin);
    //cout << num_stripes << endl;
    typedef vector<pair<int, int> > segments_t;
    typedef unordered_map<int, segments_t> segmentsets_t;
    typedef unordered_map<int, segmentsets_t > sparse_t; 
    sparse_t plane2segments;
    
    for (int i = 0; i < num_stripes; ++i) {
        int z = read_int(fin);         
        int y = read_int(fin);
        //cout << z << " " << y << endl; 
        // will same z,y appear multiple times
        segments_t segment_list;
        int num_segments = read_int(fin);         
        for (int j = 0; j < num_segments; ++j) {
            int x1 = read_int(fin);         
            int x2 = read_int(fin);         
            segment_list.push_back(make_pair(x1, x2));
        }
        plane2segments[z][y] = segment_list;
    }
    fin.close();


    //return 0;
    // iterate each z, load appropriate slice of a given size, relabel sparsely
    for (sparse_t::iterator iter = plane2segments.begin(); iter != plane2segments.end(); ++iter) {
        // get bounds
        int maxy = 0;
        int miny = INT_MAX;
        int maxx = 0;
        int minx = INT_MAX;
        for (segmentsets_t::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2) {
            if (iter2->first < miny) {
                miny = iter2->first;
            }
            if (iter2->first > maxy) {
                maxy = iter2->first;
            }
            segments_t& segment_list = iter2->second;

            for (int i = 0; i < segment_list.size(); ++i) {
                if (segment_list[i].first < minx) {
                    minx = segment_list[i].first;
                }
                if (segment_list[i].second > maxx) {
                    maxx = segment_list[i].second;
                }
            }
        }
        //cout << "z: " << iter->first << " y: " << miny << " x: " << minx << endl;
        //cout << maxx-minx+1 << "x" << maxy-miny+1 << "x1" << endl;

        // create dvid size and start points (X, Y, Z)
        libdvid::Dims_t sizes; sizes.push_back(maxx-minx+1);
        sizes.push_back(maxy-miny+1); sizes.push_back(1);
        
        vector<unsigned int> start; start.push_back(minx);
        start.push_back(miny); start.push_back(iter->first);

        // retrieve dvid slice
        libdvid::Labels3D label_data = dvid_node.get_labels3D(label_name, sizes, start, false);
        
        unsigned long long* ldata_raw = (unsigned long long*) label_data.get_raw();
        int width = maxx-minx+1;

        // rewrite body id in label data
        for (segmentsets_t::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2) {
            segments_t& segment_list = iter2->second;
            int offset = (iter2->first - miny)*width;
            for (int i = 0; i < segment_list.size(); ++i) {
                for (int j = segment_list[i].first; j <= segment_list[i].second; ++j) {
                    ldata_raw[offset + (j-minx)] = new_body_id;
                }
            }
        }

        // write new volume back
        libdvid::Labels3D labelbin = libdvid::Labels3D(ldata_raw,
                sizeof(unsigned long long)*(width)*(maxy-miny+1), sizes);
        
        dvid_node.put_labels3D(label_name, labelbin, start, false);
    }

    return 0;
}



