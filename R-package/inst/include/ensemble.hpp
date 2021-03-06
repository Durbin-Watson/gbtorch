// ensemble.hpp

#ifndef __ENSEMBLE_HPP_INCLUDED__
#define __ENSEMBLE_HPP_INCLUDED__


#include "tree.hpp"
#include "loss_functions.hpp"



//' @export ENSEMBLE
class ENSEMBLE
{
public:
    double initialPred;
    double learning_rate;
    double initial_score;
    GBTREE* first_tree;
    Rcpp::List param;
    
    // constructors
    ENSEMBLE();
    ENSEMBLE(double learning_rate_);
    
    // Functions
    void set_param(Rcpp::List par_list);
    Rcpp::List get_param();
    double initial_prediction(Tvec<double> &y, std::string loss_function);
    void train(Tvec<double> &y, Tmat<double> &X);
    Tvec<double> predict(Tmat<double> &X);
    Tvec<double> predict2(Tmat<double> &X, int num_trees);
    double get_ensemble_bias(int num_trees);
    int get_num_trees();
};


ENSEMBLE::ENSEMBLE(){
    this->learning_rate=0.01;
    this->param = Rcpp::List::create(
        Named("learning_rate")  = 0.01,
        Named("loss_function")  = "mse",
        Named("nrounds") = 5000
    );
}

ENSEMBLE::ENSEMBLE(double learning_rate_){
    this->first_tree = NULL;
    this->learning_rate = learning_rate_;
    this->param = Rcpp::List::create(
        Named("learning_rate") = learning_rate_,
        Named("loss_function") = "mse",
        Named("nrounds") = 5000
    );
}

void ENSEMBLE::set_param(Rcpp::List par_list){
    this->param = par_list;
    this->learning_rate = par_list["learning_rate"];
}
Rcpp::List ENSEMBLE::get_param(){
    return this->param;
}

double ENSEMBLE::initial_prediction(Tvec<double> &y, std::string loss_function){
    
    double pred;
    int n = y.size();
    
    if(loss_function=="mse"){
        pred = y.sum() / n;
    }else if(loss_function=="logloss"){
        double pred_g_transform = y.sum()/n; // naive probability
        pred = log(pred_g_transform) - log(1 - pred_g_transform);
    }
    
    return pred;
}


void ENSEMBLE::train(Tvec<double> &y, Tmat<double> &X){
    // Set init -- mean
    int MAXITER = param["nrounds"];
    int NUM_BINTREE_CONSECUTIVE = 0;
    double MAX_NUM_BINTREE_CONSECUTIVE = 2 / learning_rate; // Logic: if learning_rate=1 and bintree, then should be no more splits
    int NUM_LEAVES;
    int n = y.size();
    //int m = X.size();
    double EPS = -1E-12;
    double expected_loss;
    double learning_rate_set = this->learning_rate;
    Tvec<double> pred(n), g(n), h(n);
    
    // MSE -- FIX FOR OTHER LOSS FUNCTIONS
    this->initialPred = this->initial_prediction(y, param["loss_function"]); //y.sum()/n;
    pred.setConstant(this->initialPred);
    this->initial_score = loss(y, pred, param["loss_function"]); //(y - pred).squaredNorm() / n;
    
    // First tree
    g = dloss(y, pred, param["loss_function"]);
    h = ddloss(y, pred, param["loss_function"]);
    this->first_tree = new GBTREE;
    this->first_tree->train(g, h, X);
    GBTREE* current_tree = this->first_tree;
    pred = pred + learning_rate * (this->first_tree->predict_data(X)); // POSSIBLY SCALED
    expected_loss = (current_tree->getTreeScore()) * (-2)*learning_rate_set*(learning_rate_set/2 - 1) + 
        learning_rate_set * current_tree->getTreeBiasFullEXM();
    //( std::max(1.0, log(m+1e-4)) * current_tree->getTreeBiasFull() + current_tree->getTreeBias());// obs_loss + bias -- POSSIBLY SCALED
    
    // COUT
    // std::cout<< "learning rate: " << learning_rate << "\n" <<
    //     "initial prediction: " << (this->initialPred) << "\n" <<
    //         "iteration: " << 1 << "\n" << 
    //             "expected_loss: " << expected_loss << "\n" << std::endl;
    
    
    
    for(int i=2; i<(MAXITER+1); i++){
        
        // TRAINING
        GBTREE* new_tree = new GBTREE();
        g = dloss(y, pred, param["loss_function"]);
        h = ddloss(y, pred, param["loss_function"]);
        new_tree->train(g, h, X);
        
        // EXPECTED LOSS
        expected_loss = (new_tree->getTreeScore()) * (-2)*learning_rate_set*(learning_rate_set/2 - 1) + 
            learning_rate_set * new_tree->getTreeBiasFullEXM();
        //(new_tree->getTreeBiasFull() * std::max(1.0, log(m+1e-4)) + new_tree->getTreeBias());// obs_loss + bias -- POSSIBLY SCALED
        
        // // CHECKING IF BINARY TREE
        // NUM_LEAVES = new_tree->getNumLeaves();
        // if(NUM_LEAVES < 3){
        //     NUM_BINTREE_CONSECUTIVE++;
        // }else{
        //     NUM_BINTREE_CONSECUTIVE = 0;
        // }
        
        // std::cout << "iteration " << 
        //     i << "\n" << 
        //     "Num leaves: " << new_tree->getNumLeaves() << "\n" <<
        //         "expected_loss: " << expected_loss << "\n" << std::endl;
        
        
        if(expected_loss < EPS){ // && NUM_BINTREE_CONSECUTIVE < MAX_NUM_BINTREE_CONSECUTIVE){
            //if(new_tree->getNumLeaves() == 1){
            current_tree->next_tree = new_tree;
            pred = pred + learning_rate * (current_tree->predict_data(X)); // POSSIBLY SCALED
            current_tree = new_tree;
        }else{
            break;
        }
    }
}

Tvec<double> ENSEMBLE::predict(Tmat<double> &X){
    int n = X.rows();
    Tvec<double> pred(n);
    pred.setConstant(this->initialPred);
    GBTREE* current = this->first_tree;
    while(current != NULL){
        pred = pred + (this->learning_rate) * (current->predict_data(X));
        current = current->next_tree;
    }
    return pred;
}

Tvec<double> ENSEMBLE::predict2(Tmat<double> &X, int num_trees){
    int n = X.rows();
    int tree_num = 1;
    
    Tvec<double> pred(n);
    pred.setConstant(this->initialPred);
    GBTREE* current = this->first_tree;
    
    
    if(num_trees < 1){
        while(current != NULL){
            pred = pred + (this->learning_rate) * (current->predict_data(X));
            current = current->next_tree;
        }
    }else{
        while(current != NULL){
            pred = pred + (this->learning_rate) * (current->predict_data(X));
            current = current->next_tree;
            tree_num++;
            if(tree_num > num_trees) break;
        }
    }
    
    return pred;
}

double ENSEMBLE::get_ensemble_bias(int num_trees){
    
    int tree_num = 1;
    double total_observed_reduction = 0.0;
    double total_optimism = 0.0;
    double learning_rate = this->learning_rate;
    GBTREE* current = this->first_tree;
    
    if(num_trees<1){
        while(current != NULL){
            total_observed_reduction += current->getTreeScore();
            total_optimism += current->getTreeBiasFullEXM();
            current = current->next_tree;
        }
    }else{
        while(current != NULL){
            total_observed_reduction += current->getTreeScore();
            total_optimism += current->getTreeBiasFullEXM();
            current = current->next_tree;
            tree_num++;
            if(tree_num > num_trees) break;
        }
    }
    //std::cout<< (this->initial_score) << std::endl;
    return (this->initial_score) + total_observed_reduction * (-2)*learning_rate*(learning_rate/2 - 1) + 
        learning_rate * total_optimism;
    
}

int ENSEMBLE::get_num_trees(){
    int num_trees = 0;
    GBTREE* current = this->first_tree;
    
    while(current != NULL){
        num_trees++;
        current = current->next_tree;
    }
    
    return num_trees;
}



#endif