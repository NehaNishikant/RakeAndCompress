#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <future>
#include <mutex>
#include<unordered_set>
#include <time.h>
#include <string>
#include <vector>
#include <iostream>
#include <memory>

using namespace std;


struct node_data //all leaves are nums, else: op
{
    //leaves (0 if non-leaf):
    int num;

    //non leaves (undefined if leaf):
    bool op; //true= +, false= *
    int alpha; //todo: initialize to 1
    int beta; //todo: initialize to 0
};

struct node //Tree representation 
{
    struct node_data* data;
    
    struct node* left;
    struct node* right;
    struct node* parent;

    //for rake when 2 children try to rake up to same parent
    // int mutex_id;
    mutex* lock;

    //for compress to decide which nodes to comprress
    bool coinflip; 
};
typedef struct node* Tree_Node;


class ExpressionTreeSolve {
    public:
        Tree_Node Tree;
        unordered_set<Tree_Node> Nodes;
        unordered_set<Tree_Node> Leaves;
        //vector<mutex> mutexes;

        //INITIALIZE AND INITIALIZE METDATA HELPERS
        void init() {
            initialize_nodes(Tree);
        };

        void initialize_nodes(Tree_Node T) {
            if (T == NULL) {
                return;
            }
            else {
                Nodes.insert(T);
                //mutex new_m;
                //mutexes.push_back(new_m);

                if (T->left == NULL && T->right == NULL){
                    Leaves.insert(T);
                }
                initialize_nodes(T->left);
                initialize_nodes(T->right);
            }
        }

        // FREE HELPER
        void free_node(Tree_Node node){
            free(node->data);
            // free(node->lock);
            free(node);
        }

        bool validate_tree(Tree_Node T){
            if (T == NULL){
                return true;
            }

            if (T->left != NULL){
                if (T->left->parent != T){
                    cout << "alert!!\n";
                    print_node(T->left);
                    return false;
                }
            }
            if (T->right != NULL){
                if (T->right->parent != T){
                    cout << "alert!!\n";
                    print_node(T->right);
                    return false;
                }
            }
            return validate_tree(T->left) && validate_tree(T->right);
        }

        void print_node(Tree_Node T){
            if (T == NULL){
                cout << "empty\n";
                return;
            }

            if (T->right == NULL && T->left == NULL){ //leaf
                cout << "num node ";
                cout << T->data->num;
                cout << "\n";
            } else {
                cout << "inner node (a, b, num) = ";
                cout << T->data->alpha;
                cout << ", ";
                cout << T->data->beta;
                cout << ", ";
                cout << T->data->num;
                cout << "\n";
            }
        }

        void print_tree_help(Tree_Node T){
            print_node(T);
            if (T == NULL){
                return;
            }
            print_tree_help(T->left);
            print_tree_help(T->right);
        }

        void print_tree(Tree_Node T){ //in order print tree
            cout << "printing tree\n";
            print_tree_help(Tree);
            cout  << "done\n";
        }

        //RAKE AND RAKE HELPERS
        void rake_thread(promise<Tree_Node> && p, Tree_Node leaf){

            //get args
            Tree_Node parent = ((Tree_Node)leaf)->parent;
            int leaf_num = ((Tree_Node)leaf)->data->num;

            
            //lock parent_lock (another child might get there first)
            mutex* parent_lock = parent->lock;
            parent_lock->lock();

            Tree_Node new_leaf = NULL;
            
            if (parent->left != NULL & parent->right != NULL){ //first leaf (update lambda)
                if (parent->data->op){
                    //plus
                    parent->data->beta += leaf_num;
                } else{
                    //multiplication
                    parent->data->alpha *= leaf_num;
                    parent->data->beta *= leaf_num;
                }
            } else { //second leaf (resolve lambda)
                parent->data->num += parent->data->alpha * leaf_num + parent->data->beta;
                new_leaf = parent;
            }
            
            //clean up leaf
            if (parent->right == leaf){
                parent->right = NULL;
            } else{
                parent->left = NULL;
            }

            //free_node((Tree_Node)leaf); //TODO: uncomment?
            
            Nodes.erase((Tree_Node)leaf);
            //unlock parent_lock
            parent_lock->unlock();
            //return parent if it's a new leaf
            p.set_value(new_leaf);
        }

        void rake(){ //to rake is all the leaves

            // cout  << "raking\n";

            //spawn threads to do all leaves in parallel
            std::vector<std::thread> threads;
            vector< std::future<Tree_Node> > futures; //to collect parents of raked leaves
            
            //iterate through leaves
            for (unordered_set<Tree_Node>::iterator leaf = Leaves.begin(); leaf != Leaves.end(); ++leaf) {
                //spawn thread
                std::promise<Tree_Node> p;
                futures.push_back(p.get_future());
                threads.push_back(thread(&ExpressionTreeSolve::rake_thread, this, move(p), (Tree_Node)*leaf));
            }
            //wait for threads to finish
            for (auto& th : threads) th.join();

            //add parent of raked leaves to Leaves
            Leaves.clear();
            for (auto& f : futures) {
                Tree_Node new_leaf = f.get();
                if (new_leaf != NULL){
                    Leaves.insert(new_leaf);
                }
            }
        }

        //COMPRESS + COMPRESS HELPERS
        void assign_coinflips(Tree_Node tree_node) {
            ((Tree_Node)tree_node)->coinflip = (bool)(rand() % 2);
        }

        bool should_compress(Tree_Node tree_node) {
            // im a head
            bool me_head = tree_node->coinflip;
            // not root and parent tail
            Tree_Node parent = tree_node->parent;
            bool parent_tail = parent != NULL && !parent->coinflip;
            // left child tail (if it exists)
            bool left_tail = tree_node->left == NULL || !tree_node->left->coinflip;
            // right child tail (if it exists)
            bool right_tail = tree_node->right == NULL || !tree_node->right->coinflip;
            // i have exactly one child (being a leaf complicates things)
            bool one_child = (tree_node->right == NULL || tree_node->left == NULL) && (tree_node->right != NULL || tree_node->left != NULL); 
            // im an only child
            bool only_child = parent != NULL && (parent->right == NULL || parent->left == NULL);
            // all above true for compress
            return me_head && parent_tail && left_tail && right_tail && one_child; 
        }

        void compress_thread(Tree_Node tree_node) {

            // based on coinflips, only compress selected nodes
            if (!should_compress((Tree_Node)tree_node)){ 
                return;
            }

            // cout << "compressing\n";
            
            // combine alpha beta of node with parent node's alpha beta
            Tree_Node node = (Tree_Node)tree_node;
            Tree_Node parent = ((Tree_Node)tree_node)->parent;
            parent->data->alpha *= node->data->alpha;
            parent->data->beta = parent->data->alpha * node->data->beta + parent->data->beta;
            
            // restructure tree 
            if (tree_node->left != NULL){
                tree_node->left->parent = parent;
            }
            if (tree_node->right != NULL){
                tree_node->right->parent = parent;
            }
            parent->left = ((Tree_Node)tree_node)->left;
            parent->right = ((Tree_Node)tree_node)->right;

            
            
            // remove and free node. TODO: garbage collection
            Nodes.erase((Tree_Node)tree_node);
            free_node((Tree_Node)tree_node);
        }

        void compress(){ //TODO: free threads everywhere
            
            std::vector<std::thread> coinflip_threads;

            //iterate through nodes
            for (unordered_set<Tree_Node>::iterator tree_node = Nodes.begin(); tree_node != Nodes.end(); ++tree_node) {
                //spawn thread
                coinflip_threads.push_back(thread(&ExpressionTreeSolve::assign_coinflips, this, (Tree_Node)*tree_node));
            }

            //wait for threads to finish
            for (auto& th : coinflip_threads) th.join();

            //spawn threads to do all leaves in parallel
            std::vector<std::thread> compress_threads;

            //iterate through nodes
            for (unordered_set<Tree_Node>::iterator tree_node = Nodes.begin(); tree_node != Nodes.end(); ++tree_node) {
                //spawn thread
                compress_threads.push_back(thread(&ExpressionTreeSolve::compress_thread, this, (Tree_Node)*tree_node));
            }

            //wait for threads to finish
            for (auto& th : compress_threads) th.join();

        }

        //SOLVE
        int solve(){ 
            while (Nodes.size() > 1) {
                rake();
                // print_tree(Tree);
                assert(validate_tree(Tree));
                compress();
                // print_tree(Tree);
                assert(validate_tree(Tree));
            }

            Nodes.clear();
            Leaves.clear();
            int x = Tree->data->num;
            //cout << x;
            //TODO: free tree
            return x;
        }

        Tree_Node make_node_from_char(string a) {
            if (a == "NULL") {
                return NULL;
            }
            if (a == "+") {
                return make_op_node(true);
            }
            if (a == "*"){
                return make_op_node(false);
            }
            return make_num_node(stoi(a));
        }

        //INPUT TREE INITIALIZATION HELPERS
        void make_tree_from_list(vector<string>& chars, int len) {
            //make a list to keep track of nodes indices
            Tree_Node* list_of_nodes = (Tree_Node *)calloc(len, sizeof(Tree_Node));
            for(int j = 0; j < chars.size(); j++){
                Tree_Node node = make_node_from_char(chars[j]);

                int i = j+1;
                Tree_Node parent = NULL;
                if (i != 1) {
                    parent = list_of_nodes[i/2 - 1];
                }

                if (node != NULL){
                    node->parent = parent;
                    node->left = NULL;
                    node->right = NULL;
                }
                
                if (parent != NULL){
                    if (i % 2) { // right child
                        parent->right = node;
                    }
                    else { // left child
                        parent->left = node;
                    }
                }
                list_of_nodes[j] = node;
            }
            Tree = list_of_nodes[0];
        }
        
        Tree_Node make_node(struct node_data* data){
            //mutex lock;
            Tree_Node tree_node = (Tree_Node)malloc(sizeof(struct node));
            tree_node->data = data;
            tree_node->lock = new mutex();
            return tree_node; //does not initialize parent and child pointers
        }

        Tree_Node make_op_node(bool plus){
            struct node_data* data = (struct node_data*)(malloc(sizeof(struct node_data)));
            data->op = plus;
            data->alpha = 1;
            data->beta = 0;
            data->num = 0;
            return make_node(data);
        }

        Tree_Node make_num_node(int num){
            struct node_data* data = (struct node_data*)malloc(sizeof(struct node_data));
            data->num = num;
            return make_node(data);
        }

};

int main(int argc, char** argv){
    //test cases
    ExpressionTreeSolve Solver = ExpressionTreeSolve();

    // 1 (tests singleton leaf)
    vector<string> tree1;
    tree1.push_back("1");
    Solver.make_tree_from_list(tree1, 1);

    Solver.init();
    assert(Solver.solve() == 1);
    cout << "Test case 1 passed\n";

    //2 (tests single rake + )
    vector<string> tree2;
    tree2.push_back("+");
    tree2.push_back("5");
    tree2.push_back("6");
    Solver.make_tree_from_list(tree2, 3);
    Solver.init();
    assert(Solver.solve() == 11);
    cout << "Test case 2 passed\n";

    //3 (tests single rake *)
    vector<string> tree3;
    tree3.push_back("*");
    tree3.push_back("5");
    tree3.push_back("6");
    Solver.make_tree_from_list(tree3, 3);
    Solver.init();
    assert(Solver.solve() == 30);
    cout << "Test case 3 passed\n";

    //4 (tests single rake + long chain)
    vector<string> tree4;
    int num_levels = 7;
    int next_po2 = 1;
    int i =0;
    int counter = 1;
    while (i < pow(2, num_levels-1)+1){
        if (i+1==1){
            tree4.push_back("+");
            i +=1;
            next_po2 *=2;
        } else{
            if (i+1==pow(2, num_levels-1)){
                tree4.push_back(to_string(counter));
                tree4.push_back(to_string(counter+1));
                i+=2;
                counter+=2;
            } else {
                if ((i+1) == next_po2){
                    tree4.push_back("+");
                    tree4.push_back(to_string(counter));
                    i +=2;
                    counter +=1;
                    next_po2 *=2;
                } else {
                    tree4.push_back("NULL");
                    i+=1;
                }
            }
        }
    }
    Solver.make_tree_from_list(tree4, pow(2, num_levels-1)+1);
    Solver.init();
    assert(Solver.solve() == ((1+num_levels)*(num_levels))/2);
    cout << "Test case 4 passed\n";

    //5 long chain with + and *
    vector<string> tree5;
    tree5.push_back("+");

    tree5.push_back("*");
    tree5.push_back("6");

    tree5.push_back("+");
    tree5.push_back("5");
    for (int i=0; i<2; i++){
        tree5.push_back("NULL");
    }
    tree5.push_back("*");
    tree5.push_back("4");
    for (int i=0; i<6; i++){
        tree5.push_back("NULL");
    }
    tree5.push_back("+");
    tree5.push_back("3");
    for (int i=0; i<14; i++){
        tree5.push_back("NULL");
    }
    tree5.push_back("2");
    tree5.push_back("1");

    Solver.make_tree_from_list(tree5, 33);
    Solver.init();
    assert(Solver.solve() == 71);
    cout << "Test case 5 passed\n";
}