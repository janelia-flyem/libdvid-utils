#include <json/json.h>
#include <json/value.h>

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

const char * USAGE = "<prog> <dvid-server> <uuid> <graph-name> <synapse-file> <label name>";
const char * HELP = "Program takes a synapse file (in global DVID coordinates) and saves the counts and partners in the given graph";
static const char * SYNAPSE_KEY = "synapse";

int main(int argc, char** argv)
{
    if (argc != 6) {
        cout << USAGE << endl;
        cout << HELP << endl;
        exit(1);
    }
    
    // create DVID node accessor 
    libdvid::DVIDNodeService dvid_node(argv[1], argv[2]);

    string graph_name = string(argv[3]);
   
    unordered_map<unsigned long long, unsigned long long> counts;
    unordered_map<unsigned long long, unordered_set<unsigned long long> > partners;

    // size and channel params for standard label rest call
    libdvid::Dims_t sizes; sizes.push_back(1);
    sizes.push_back(1); sizes.push_back(1);


    // read synapse file
    Json::Reader json_reader;
    Json::Value json_reader_vals;
    ifstream fin(argv[4]);
    if (!fin) {
        cout << "Error: input file: " << argv[4] << " cannot be opened" << endl;
        exit(1);
    }
    if (!json_reader.parse(fin, json_reader_vals)) {
        cout << "Error: Json incorrectly formatted" << endl;
        exit(1);
    }
    fin.close();
   
    // iterate all synapses 
    Json::Value synapses = json_reader_vals["data"];
    for (int i = 0; i < synapses.size(); ++i) {
        vector<unsigned int> constraint_list;

        Json::Value location = synapses[i]["T-bar"]["location"];
        if (!location.empty()) {
            int xloc = location[(unsigned int)(0)].asUInt();
            int yloc = location[(unsigned int)(1)].asUInt();
            int zloc = location[(unsigned int)(2)].asUInt();
            
            // determine label corresponding to point
            vector<unsigned int> start; start.push_back(xloc);
            start.push_back(yloc); start.push_back(zloc);
            
            // retrieve volume 
            libdvid::Labels3D labels = dvid_node.get_labels3D(argv[5], sizes, start);
            unsigned long long* ptr = (unsigned long long int*) labels.get_raw();
            unsigned long long label = *ptr;

            if (label) {
                constraint_list.push_back(label);
                counts[label]++;
            }
        }
        Json::Value psds = synapses[i]["partners"];
        for (int i = 0; i < psds.size(); ++i) {
            Json::Value location = psds[i]["location"];
            if (!location.empty()) {
                int xloc = location[(unsigned int)(0)].asUInt();
                int yloc = location[(unsigned int)(1)].asUInt();
                int zloc = location[(unsigned int)(2)].asUInt();
            
                // determine label corresponding to point
                vector<unsigned int> start; start.push_back(xloc);
                start.push_back(yloc); start.push_back(zloc);

                // retrieve volume 
                libdvid::Labels3D labels = dvid_node.get_labels3D(argv[5], sizes, start);
                unsigned long long* ptr = (unsigned long long int*) labels.get_raw();
                unsigned long long label = *ptr;

                if (label) {
                    constraint_list.push_back(label);
                    counts[label]++;
                }
            }
        }
       
        // load constraints for Tbar to PSD and PSD to PSD 
        for (int it1 = 0; it1 < constraint_list.size(); ++it1) {
            for (int it2 = (it1+1); it2 < constraint_list.size(); ++it2) {
                if (constraint_list[it1] != constraint_list[it2]) {
                    partners[(constraint_list[it1])].insert((constraint_list[it2]));
                    partners[(constraint_list[it2])].insert((constraint_list[it1]));
                }
            }
        }
    }
    cout << "Finished reading all synapses" << endl;

    // load vertex list and data
    vector<libdvid::Vertex> vertices;
    vector<libdvid::BinaryDataPtr> properties;
    libdvid::VertexTransactions transaction_ids; 

    // load property data for post
    for (unordered_map<unsigned long long, unsigned long long>::iterator iter =
            counts.begin(); iter != counts.end(); ++iter) {
        vertices.push_back(libdvid::Vertex(iter->first, 0));

        // set count and constraints 
        unsigned long long num_partners = partners[iter->first].size();
        int total_size = num_partners + 2;  
        assert(sizeof(unsigned long long) == 8);

        unsigned long long* data = new unsigned long long [total_size]; 
        
        int spot = 0; 
        data[spot] = iter->second;
        ++spot;
        data[spot] = num_partners;
        ++spot;
        for (unordered_set<unsigned long long>::iterator iter2 =
            partners[iter->first].begin(); iter2 != partners[iter->first].end(); ++iter2) {
            data[spot] = *iter2;
            ++spot;
        }

        properties.push_back(libdvid::BinaryData::create_binary_data((char*) data,
                    total_size*8));

        delete []data;
    }
    cout << "Finished processing all constraints" << endl;

    // nominally use transaction protection to load data; this should be run by itself
    vector<libdvid::BinaryDataPtr> properties_dummy;
    // retrieve vertex transactions
    dvid_node.get_properties(graph_name, vertices, SYNAPSE_KEY,
            properties_dummy, transaction_ids);

    // set vertex transactions
    vector<libdvid::Vertex> leftover_vertices;
    dvid_node.set_properties(graph_name, vertices, SYNAPSE_KEY, properties,
            transaction_ids, leftover_vertices);

    assert(leftover_vertices.size() == 0);

    return 0;
}



