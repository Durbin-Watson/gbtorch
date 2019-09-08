#' GBTorch Training.
#'
#' \code{gbt.train} is an interface for training a \code{gbtorch} model.
#'
#' @param param the list of parameters:
#'
#' 1. Task Parameters
#'
#' \itemize{
#'   \item \code{loss_function} specify the learning objective (loss function). Only pre-specified loss functions are currently supported.
#'   \itemize{
#'   \item \code{mse} regression with squared error loss (Default).
#'   \item \code{logloss} logistic regression for binary classification, output score before logistic transformation.
#'   }
#' }
#'
#' 2. Tree Booster Parameters
#'
#' \itemize{
#'   \item \code{learning_rate} control the learning rate: scale the contribution of each tree by a factor of \code{0 < learning_rate < 1} when it is added to the current approximation. Lower value for \code{learning_rate} implies an increase in the number of boosting iterations: low \code{learning_rate} value means model more robust to overfitting but slower to compute. Default: 0.01
#'   \item \code{nrounds} a just-in-case max number of boosting iterations. Default: 5000
#' }
#'
#'
#' @param y response vector for training. Must correspond to the design matrix \code{x}.
#' @param x design matrix for training. Must be of type \code{matrix}.
#'
#' @details
#' These are the training functions for \code{gbtorch}.
#' 
#' Explain the philosophy and the algorithm and a little math
#'
#' @return
#' An object of class \code{ENSEMBLE} with the following elements:
#' \itemize{
#'   \item \code{handle} a handle (pointer) to the xgboost model in memory.
#'   \item \code{initialPred} a field containing the initial prediction of the ensemble.
#'   \item \code{set_param} function for changing the parameters of the ensemble.
#'   \item \code{get_param} function for looking up the parameters of the ensemble.
#'   \item \code{train} function for re-training (or from scratch) the ensemble directly on vector \code{y} and design matrix \code{x}.
#'   \item \code{predict} function for predicting observations given a design matrix
#'   \item \code{predict2} function as above, but takes a parameter max number of boosting ensemble iterations.
#'   \item \code{get_ensemble_bias} function for calculating the (approximate) optimism of the ensemble.
#'   \item \code{get_num_trees} function returning the number of trees in the ensemble.
#' }
#'
#' @seealso
#' \code{\link{gbt.pred}}
#'
#' @references
#'
#' B. Lunde, T. S. Kleppe and H. J. Skaug, "An information criterion for gradient boosted trees"
#' publishing details, \url{}
#'
#' @examples
#' ## A simple gtb.train example with linear regression:
#' x <- runif(500, 0, 4)
#' y <- rnorm(500, x, 1)
#' x.test <- runif(500, 0, 4)
#' y.test <- rnorm(500, x.test, 1)
#' 
#' param <- list("learning_rate" = 0.03, "loss_function" = "mse", "nrounds"=2000)
#' mod <- gbt.train(param, y, as.matrix(x))
#' y.pred <- mod$predict( as.matrix( x.test ) )
#' 
#' plot(x.test, y.test)
#' points(x.test, y.pred, col="red")
#'
#'
#' @rdname gbt.train
#' @export
gbt.train <- function(param = list(), y, x){
    
    # checks...
    # param, y, x
    
    # create gbtorch ensemble object
    mod <- new(ENSEMBLE)
    mod$set_param(param)
    
    # train ensemble
    mod$train(y,x)
    
    # return trained gbtorch ensemble
    return(mod)
}