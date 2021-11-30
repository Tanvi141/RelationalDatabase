#include "RecordPtr.hpp"
#include "LeafNode.hpp"

LeafNode::LeafNode(const TreePtr &tree_ptr) : TreeNode(LEAF, tree_ptr) {
    this->data_pointers.clear();
    this->next_leaf_ptr = NULL_PTR;
    if(!is_null(tree_ptr))
        this->load();
}

//returns max key within this leaf
Key LeafNode::max() {
    auto it = this->data_pointers.rbegin();
    return it->first;
}

//inserts <key, record_ptr> to leaf. If overflow occurs, leaf is split
//split node is returned
TreePtr LeafNode::insert_key(const Key &key, const RecordPtr &record_ptr) {
    TreePtr new_leaf = NULL_PTR; //if leaf is split, new_leaf = ptr to new split node ptr

    // Insert data
    this->data_pointers.insert(pair<Key,RecordPtr>(key, record_ptr));
    this->size ++ ;

    // handle overflow
    if (this->overflows()) {
        auto* split_node = new LeafNode();
        vector<Key> toDelete;
        int currIdx = 0;
        // transfer data to leaf split_node
        for (auto it : this->data_pointers) {
            if (currIdx >= MIN_OCCUPANCY) {
                split_node->insert_key(it.first, it.second);
                toDelete.push_back(it.first);
            }
            currIdx ++ ;
        }
        // delete transfered data from current leaf node
        for (auto it : toDelete)
            this->data_pointers.erase(it);

        new_leaf = split_node->tree_ptr;
        this->size = MIN_OCCUPANCY;
        toDelete.clear();

        // change leaf pointers
        split_node->next_leaf_ptr = this->next_leaf_ptr;
        this->next_leaf_ptr = split_node->tree_ptr;
        split_node->dump();
        delete split_node;
    }

    this->dump();
    return new_leaf;
}

//key is deleted from leaf if exists
void LeafNode::delete_key(const Key &key) {

    // key doesn't exist
    if (this->data_pointers.find(key) == this->data_pointers.end())
        return ;

    // key exists, so remove it
    this->data_pointers.erase(key);
    this->size -- ;

    this->dump();
}

//runs range query on leaf
void LeafNode::range(ostream &os, const Key &min_key, const Key &max_key) const {
    BLOCK_ACCESSES++;
    for(const auto& data_pointer : this->data_pointers){
        if(data_pointer.first >= min_key && data_pointer.first <= max_key)
            data_pointer.second.write_data(os);
        if(data_pointer.first > max_key)
            return;
    }
    if(!is_null(this->next_leaf_ptr)){
        auto next_leaf_node = new LeafNode(this->next_leaf_ptr);
        next_leaf_node->range(os, min_key, max_key);
        delete next_leaf_node;
    }
}

//exports node - used for grading
void LeafNode::export_node(ostream &os) {
    TreeNode::export_node(os);
    for(const auto& data_pointer : this->data_pointers){
        os << data_pointer.first << " ";
    }
    os << endl;
}

//writes leaf as a mermaid chart
void LeafNode::chart(ostream &os) {
    string chart_node = this->tree_ptr + "[" + this->tree_ptr + BREAK;
    chart_node += "size: " + to_string(this->size) + BREAK;
    for(const auto& data_pointer: this->data_pointers) {
        chart_node += to_string(data_pointer.first) + " ";
    }
    chart_node += "]";
    os << chart_node << endl;
}

ostream& LeafNode::write(ostream &os) const {
    TreeNode::write(os);
    for(const auto & data_pointer : this->data_pointers){
        if(&os == &cout)
            os << "\n" << data_pointer.first << ": ";
        else
            os << "\n" << data_pointer.first << " ";
        os << data_pointer.second;
    }
    os << endl;
    os << this->next_leaf_ptr << endl;
    return os;
}

istream& LeafNode::read(istream& is){
    TreeNode::read(is);
    this->data_pointers.clear();
    for(int i = 0; i < this->size; i++){
        Key key = DELETE_MARKER;
        RecordPtr record_ptr;
        if(&is == &cin)
            cout << "K: ";
        is >> key;
        if(&is == &cin)
            cout << "P: ";
        is >> record_ptr;
        this->data_pointers.insert(pair<Key,RecordPtr>(key, record_ptr));
    }
    is >> this->next_leaf_ptr;
    return is;
}