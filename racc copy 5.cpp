#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include<unordered_set>
#include <time.h>

using namespace std;

struct node_data //all leaves are nums, else: lambda
{
    bool op; //true= +, false= *, undefined if not op
    int num; //undefined if is op

    int alpha; //todo: initialize to 1
    int beta; //todo: initialize to 0
};

struct node //Tree representation 
{
    struct node_data* data;
    
    struct node* left;
    struct node* right;
    struct node* parent;

    pthread_mutex_t *lock; 
    bool coinflip;
};
typedef struct node* Tree_Node;


class ExpressionTreeSolve {
    public:
        Tree_Node Tree;
        std::unordered_set<Tree_Node> Nodes = {};
        std::unordered_set<Tree_Node> Leaves = {};

        void init(Tree_Node T) {
            Tree = T;
            initialize_nodes(T);
            initialize_leaves(T);
        };

        int solve(Tree_Node root, int l){ 
            while () 
            rake();
            compress();
        }

        void initialize_nodes(Tree_Node T) {
            if (T == NULL) {
                return;
            }
            else {
                Nodes.insert(T);
                initialize_nodes(T->left);
                initialize_nodes(T->right);
            }
        }

        // initialize set of leaves
        void initialize_leaves(Tree_Node T) {
            if (T == NULL) {
                return;
            } 
            if (T->left == NULL && T->right == NULL) {
                Leaves.insert(T);
            }
            else {
                initialize_leaves(T->left);
                initialize_leaves(T->right);
            }
        }

        void free_node(Tree_Node node){
            free(node->data);
            free(node->lock);
            free(node);
        }

        void* rake_thread(void* leaf){
            //get args
            Nodes.erase((Tree_Node)leaf);
            
            Tree_Node parent = ((Tree_Node)leaf)->parent;
            pthread_mutex_t* parent_lock = parent->lock;
            int leaf_num = ((Tree_Node)leaf)->data->num;
            
            //lock parent_lock (another child might get there first)
            pthread_mutex_lock(parent_lock); 
            
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
            }
            
            //clean up leaf
            if (parent->right == leaf){
                parent->right = NULL;
            } else{
                parent->left = NULL;
            }
            free_node((Tree_Node)leaf);
            

            //unlock parent_lock
            pthread_mutex_unlock(parent_lock); 
        }

        void rake(){ //to rake is all the leaves

            // Create local copy of current leaves and empty global leaves for newly formed leaves to be inserted 
            std::unordered_set<Tree_Node> Leaves_local = Leaves;
            Leaves.clear();

            //spawn threads to do all leaves in parallel
            pthread_t* threads = (pthread_t*)calloc(Leaves_local.size(), sizeof(pthread_t));

            //iterate through leaves
            for (unordered_set<Tree_Node>::iterator leaf = Leaves_local.begin(); leaf != Leaves_local.end(); ++leaf) {
                //spawn thread
                pthread_t thread_id;
                *threads = thread_id;
                threads = threads + sizeof(pthread_t); //keep track of threads to join later
                pthread_create(&thread_id, NULL, (void*(*)(void*)) &ExpressionTreeSolve::rake_thread, (void*)(&(*leaf)));
            }
            //join threads (waits)
            for (int i=0; i<Leaves_local.size(); i++){
                pthread_join(threads[i], NULL);
            }
        }

        void *assign_coinflips(void* tree_node) {
            ((Tree_Node)tree_node)->coinflip = (bool)(rand() % 2);
        }

        bool should_compress(Tree_Node tree_node) {
            // im a head
            bool me_head = tree_node->coinflip;
            // parent tail
            Tree_Node parent = tree_node->parent;
            bool parent_tail = parent != NULL && !parent->coinflip;
            // left child tail (if it exists)
            bool left_tail = tree_node->left == NULL || !tree_node->left->coinflip;
            // right child tail (if it exists)
            bool right_tail = tree_node->right == NULL || !tree_node->right->coinflip;
            // i have exactly one child (being a leaf complicates things)
            bool one_child = (tree_node->right == NULL || tree_node->left == NULL) && (tree_node->right != NULL || tree_node->left != NULL); 
            // im an only child
            bool only_child = parent->right == NULL || parent->left == NULL;
            // all above true for compress
            return me_head && parent_tail && left_tail && right_tail && one_child; 
        }

        void *compress_thread(void *tree_node) {
            // based on coinflips, only compress selected nodes
            if (!should_compress((Tree_Node)tree_node)){ 
                return;
            }
            
            // combine alpha beta of node with parent node's alpha beta
            Tree_Node node = (Tree_Node)tree_node;
            Tree_Node parent = ((Tree_Node)tree_node)->parent;
            parent->data->alpha *= node->data->alpha;
            parent->data->beta *= parent->data->alpha * node->data->beta + parent->data->beta;
            
            // restructure tree 
            parent->left = ((Tree_Node)tree_node)->left;
            parent->right = ((Tree_Node)tree_node)->right;
            
            // remove and free node. TODO: garbage collection
            Nodes.erase((Tree_Node)tree_node);
            free_node((Tree_Node)tree_node);
        }

        void compress(){
            //spawn threads to do all leaves in parallel
            pthread_t* threads = (pthread_t*)calloc(Nodes.size(), sizeof(pthread_t));

            //iterate through nodes
            for (unordered_set<Tree_Node>::iterator tree_node = Nodes.begin(); tree_node != Nodes.end(); ++tree_node) {
                //spawn thread
                pthread_t thread_id;
                *threads = thread_id;
                threads = threads + sizeof(pthread_t);
                pthread_create(&thread_id, NULL, (void*(*)(void*)) &ExpressionTreeSolve::assign_coinflips, (void*)(&(*tree_node)));
            }

            //join threads
            for (int i=0; i<Nodes.size(); i++){
                pthread_join(threads[i], NULL);
            }

            //spawn threads to do all leaves in parallel
            pthread_t* threads = (pthread_t*)calloc(Nodes.size(), sizeof(pthread_t));

            //iterate through nodes
            for (unordered_set<Tree_Node>::iterator tree_node = Nodes.begin(); tree_node != Nodes.end(); ++tree_node) {
                //spawn thread
                pthread_t thread_id;
                *threads = thread_id;
                threads = threads + sizeof(pthread_t);
                pthread_create(&thread_id, NULL, (void*(*)(void*)) &ExpressionTreeSolve::compress_thread, (void*)(&(*tree_node)));
            }

            //join threads
            for (int i=0; i<Nodes.size(); i++){
                pthread_join(threads[i], NULL);
            }

        }

        struct node_data make_op_node_data(bool plus){
            struct node_data* data = (struct node_data*)(malloc(sizeof(struct node_data)));
            data->op = plus;
            data->alpha = 1;
            data->beta = 0;
        }
        struct node_data make_num_node_data(int num){
            struct node_data* data = (struct node_data*)(malloc(sizeof(struct node_data)));
            data->num = num;
        }
        struct 

};


int main(int argc, char* argv){
    
    //test cases
}