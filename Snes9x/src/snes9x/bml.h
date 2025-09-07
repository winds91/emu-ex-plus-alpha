#ifndef __BML_H
#define __BML_H
#include <vector>
#include <string>
#include <istream>

struct bml_node
{
    enum node_type {
        CHILD,
        ATTRIBUTE
    };

    bml_node();
    bool parse_file(const std::string &filename);
    void parse(std::istream &fd);
    bml_node *find_subnode(const std::string &name);
    void print();

    std::string name;
    std::string data;
    int depth;
    std::vector<bml_node> child;
    node_type type;
};

#endif
