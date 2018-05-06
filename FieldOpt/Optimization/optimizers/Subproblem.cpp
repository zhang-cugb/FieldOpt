
// -----------------------------------------------------------------
#include "Subproblem.h"

// -----------------------------------------------------------------
namespace Optimization {
namespace Optimizers {

// -----------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
int SNOPTusrFG3_(integer *Status, integer *n, doublereal x[],
                 integer *needF, integer *neF, doublereal F[],
                 integer *needG, integer *neG, doublereal G[],
                 char *cu, integer *lencu,
                 integer iu[], integer *leniu,
                 doublereal ru[], integer *lenru);
#ifdef __cplusplus
}
#endif

// -----------------------------------------------------------------
static double *gradient;
static double *hessian;
static double constant;

// -----------------------------------------------------------------
using std::vector;

// -----------------------------------------------------------------
//Subproblem::Subproblem(SNOPTHandler snoptHandler) {
Subproblem::Subproblem(Settings::Optimizer *settings) {
  settings_ = settings;

  loadSNOPT();
  setConstraintsAndDimensions(); // This one sets iGfun/jGvar, etc.
  setAndInitializeSNOPTParameters();

  ResetSubproblem();

}

// -----------------------------------------------------------------
SNOPTHandler Subproblem::initSNOPTHandler() {

  // ---------------------------------------------------------------
  string prnt_file, smry_file, optn_file;
  optn_file = settings_->parameters()
      .thrdps_optn_file.toStdString() + ".opt.optn";
  smry_file = settings_->parameters()
      .thrdps_smry_file.toStdString() + ".opt.summ";
  prnt_file = settings_->parameters()
      .thrdps_prnt_file.toStdString() + ".opt.prnt";

  SNOPTHandler snoptHandler(prnt_file.c_str(),
                            smry_file.c_str(),
                            optn_file.c_str());

  cout << "[opt]Init. SNOPTHandler.------" << endl;
  return snoptHandler;
}

// -----------------------------------------------------------------
void Subproblem::setAndInitializeSNOPTParameters() {

  // Controls
  x_ = new double[n_];

  // Initial guess for Lagrange multipliers
  xmul_ = new double[n_];

  // State of variables (whether x* is likely to be on the boundary or not) <-- ??
  xstate_ = new integer[n_];

  // ---------------------------------------------------------------
  // Objective
  F_ = new double[neF_];
  Fmul_ = new double[neF_];
  Fstate_ = new integer[neF_];

  // ---------------------------------------------------------------
  nxnames_ = 1;
  nFnames_ = 1;
  xnames_ = new char[nxnames_ * 8];
  Fnames_ = new char[nFnames_ * 8];

}

// -----------------------------------------------------------------
void Subproblem::Solve(vector<double> &xsol,
                       vector<double> &fsol,
                       char *optimizationType) {

  // ---------------------------------------------------------------
  // The snoptHandler must be setup and loaded
  SNOPTHandler snoptHandler = initSNOPTHandler();
  snoptHandler.setProbName("SNOPTSolverC");
  snoptHandler.setParameter(optimizationType);

  setOptionsForSNOPT(snoptHandler);

  // ---------------------------------------------------------------
  ResetSubproblem();
  passParametersToSNOPTHandler(snoptHandler);
  integer Cold = 0;
  integer Basis = 1;
  integer Warm = 2;

  snoptHandler.solve(Cold, xsol, fsol);

///*
  cout << "xsol: " << endl;
  for (int j = 0; j < n_; j++) {
    cout << xsol[j] << endl;
  }
  cout << endl << "Objective values:" << endl;
  for (int j = 0; j < neF_; j++) {
    cout << fsol[j] << endl;
  }
//*/
}

// -----------------------------------------------------------------
void Subproblem::ResetSubproblem() {

  // ---------------------------------------------------------------
  for (int i = 0; i < n_; i++) {
    Fstate_[i] = 0;
    xstate_[i] = 0;
    x_[i] = 0.0;
    xmul_[i] = 0;
  }

  // ---------------------------------------------------------------
  for (int h = 0; h < neF_; h++) {
    F_[h] = 0.0;
    Fmul_[h] = 0.0;
  }

}

// -----------------------------------------------------------------
void Subproblem::passParametersToSNOPTHandler(SNOPTHandler &snoptHandler) {

  // ---------------------------------------------------------------
  snoptHandler.setProblemSize(n_, neF_);
  snoptHandler.setObjective(objRow_);

  // ---------------------------------------------------------------
  snoptHandler.setA(lenA_, iAfun_, jAvar_, A_);
  snoptHandler.setG(lenG_, iGfun_, jGvar_);

  // ---------------------------------------------------------------
  snoptHandler.setX(x_, xlow_, xupp_, xmul_, xstate_);
  snoptHandler.setF(F_, Flow_, Fupp_, Fmul_, Fstate_);
  snoptHandler.setXNames(xnames_, nxnames_);
  snoptHandler.setFNames(Fnames_, nFnames_);

  // ---------------------------------------------------------------
  snoptHandler.setNeA(neA_);
  snoptHandler.setNeG(neG_);
  snoptHandler.setUserFun(SNOPTusrFG3_);

}

// -----------------------------------------------------------------
void Subproblem::setConstraintsAndDimensions() {

  // ---------------------------------------------------------------
  // This must be set before compiling the code. It cannot
  // be done during runtime through function calls.
  n_ = 2; // # of variables
  m_ = 1; // # of nonlinear constraints

  // ---------------------------------------------------------------
  // total # of constraints + objective (length of F vector)
  neF_ = m_ + 1; // l=2

  // ---------------------------------------------------------------
  // # of linear constraints
  lenA_ = 0;

  // ---------------------------------------------------------------
  // Length of gradient vector (dimension of iGfun jGvar arrays)
  // --> n: (length of obj.grad = # of vars) +
  // --> m * n: (# of constraints) * (# of vars)
  lenG_ = n_ + m_ * n_; // 2+1*2=4

  // ---------------------------------------------------------------
  // In theory the objective function could be any component of F.
  // Here we specify the objective is the first component of F
  objRow_ = 0;

  // ---------------------------------------------------------------
  objAdd_ = 0.0; // Add nothing to the objective for output purposes

  // ---------------------------------------------------------------
  // Allocate memory

  // Objective
  Flow_ = new double[neF_];
  Fupp_ = new double[neF_];

  // Variables
  xlow_ = new double[n_];
  xupp_ = new double[n_];


  iGfun_ = new integer[lenG_];
  jGvar_ = new integer[lenG_];

  // ---------------------------------------------------------------
  // No linear constraints

  // iAfun_ = new integer[lenA_];
  // jAvar_ = new integer[lenA_];
  // A_ = new double[lenA_];

  iAfun_ = nullptr;
  jAvar_ = nullptr;
  A_     = nullptr;

  // ---------------------------------------------------------------
  // neF_ += 2; // Two linear constraints
  // lenG_--; // We only have 1 derivative for the nonlinear
  // constraint (it doesn't depend upon both variables)

  // ---------------------------------------------------------------
  // Bounds objective
  Flow_[0] = -infinity_;
  Fupp_[0] = infinity_;
  Flow_[1] = 3;
  Fupp_[1] = 10;

  // ---------------------------------------------------------------
  // Upper bounds x1, x2
  xupp_[0] = infinity_;
  xupp_[1] = infinity_;

  // ---------------------------------------------------------------
  // Lower bounds x1, x2
  xlow_[0] = -infinity_;
  xlow_[1] = -infinity_;

  // ---------------------------------------------------------------
  // Indexing gradient vector
  iGfun_[0] = 0;
  iGfun_[1] = 0;
  iGfun_[2] = 1;
  iGfun_[3] = 1;

  // ---------------------------------------------------------------
  // Indexing function vector
  jGvar_[0] = 0;
  jGvar_[1] = 1;
  jGvar_[2] = 0;
  jGvar_[3] = 1;

  // ---------------------------------------------------------------
  // This is the nonzero structure of the Jacobian
  neG_ = lenG_;
  neA_ = lenA_;

}

// -----------------------------------------------------------------
Subproblem::~Subproblem() {
  delete[] iGfun_;
  delete[] jGvar_;
  delete[] x_;
  delete[] xlow_;
  delete[] xupp_;
  delete[] xmul_;
  delete[] xstate_;
  delete[] F_;
  delete[] Flow_;
  delete[] Fupp_;
  delete[] Fmul_;
  delete[] Fstate_;
  delete[] xnames_;
  delete[] Fnames_;
  delete[] gradient;
  delete[] hessian;
}

// -----------------------------------------------------------------
void Subproblem::setOptionsForSNOPT(SNOPTHandler &snoptHandler) {

  //if (settings_->verb_vector()[6] >= 1) // idx:6 -> opt (Optimization)
  cout << "[opt]Set options for SNOPT.---" << endl;

  //snoptHandler.setParameter("Backup basis file              0");
//  snoptHandler.setRealParameter("Central difference interval", 2 * derivativeRelativePerturbation);

  //snoptHandler.setIntParameter("Check frequency",           60);
  //snoptHandler.setParameter("Cold Start                     Cold");

  //snoptHandler.setParameter("Crash option                   3");
  //snoptHandler.setParameter("Crash tolerance                0.1");
  //snoptHandler.setParameter("Derivative level               3");

//  if ( (optdata.optimizationType == HISTORY_MATCHING) || hasNonderivativeLinesearch )
//    snoptHandler.setParameter((char*)"Nonderivative linesearch");
//  else
  //snoptHandler.setParameter((char*)"Derivative linesearch");
  //snoptHandler.setIntParameter("Derivative option", 0);

//  snoptHandler.setRealParameter("Difference interval", optdata.derivativeRelativePerturbation);

  //snoptHandler.setParameter("Dump file                      0");
  //snoptHandler.setParameter("Elastic weight                 1.0e+4");
  //snoptHandler.setParameter("Expand frequency               10000");
  //snoptHandler.setParameter("Factorization frequency        50");
  //snoptHandler.setRealParameter("Function precision", sim_data.tuningParam.tstep.minDeltat);
  //snoptHandler.setParameter("Hessian full memory");
  //snoptHandler.setParameter("Hessian limited memory");

//  snoptHandler.setIntParameter("Hessian frequency", optdata.frequencyToResetHessian);
  //snoptHandler.setIntParameter("Hessian updates", 0);
  //snoptHandler.setIntParameter("Hessian flush", 1);  // Does NOT work in the current version of SNOPT!!!

  //snoptHandler.setParameter("Insert file                    0");
//  snoptHandler.setRealParameter("Infinite bound", optdata.defaultControlUpperBound);

  //snoptHandler.setParameter("Iterations limit");
  //snoptHandler.setRealParameter("Linesearch tolerance",0.9);
  //snoptHandler.setParameter("Load file                      0");
  //snoptHandler.setParameter("Log frequency                  100");
  //snoptHandler.setParameter("LU factor tolerance            3.99");
  //snoptHandler.setParameter("LU update tolerance            3.99");
  //snoptHandler.setRealParameter("LU factor tolerance", 3.99);
  //snoptHandler.setRealParameter("LU update tolerance", 3.99);
  //snoptHandler.setParameter("LU partial pivoting");
  //snoptHandler.setParameter("LU density tolerance           0.6");
  //snoptHandler.setParameter("LU singularity tolerance       3.2e-11");

  //target nonlinear constraint violation
  //snoptHandler.setRealParameter("Major feasibility tolerance", 0.000001);
  //snoptHandler.setIntParameter("Major Iterations Limit", 1000);

  //target complementarity gap
  //snoptHandler.setRealParameter("Major optimality tolerance", 0.0001);

  //snoptHandler.setParameter("Major Print level  11111"); //  000001"
  //snoptHandler.setRealParameter("Major step limit", 0.2);
  //snoptHandler.setIntParameter("Minor iterations limit", 200); // 200

  //for satisfying the QP bounds
//  snoptHandler.setRealParameter("Minor feasibility tolerance", optdata.constraintTolerance);
  snoptHandler.setIntParameter("Minor print level", 10);
  //snoptHandler.setParameter("New basis file                 0");
  //snoptHandler.setParameter("New superbasics limit          99");
  //snoptHandler.setParameter("Objective Row");
  //snoptHandler.setParameter("Old basis file                 0");
  //snoptHandler.setParameter("Partial price                  1");
  //snoptHandler.setParameter("Pivot tolerance                3.7e-11");
  //snoptHandler.setParameter("Print frequency                100");
  //snoptHandler.setParameter("Proximal point method          1");
  //snoptHandler.setParameter("QPSolver Cholesky");
  //snoptHandler.setParameter("Reduced Hessian dimension");
  //snoptHandler.setParameter("Save frequency                 100");
  snoptHandler.setIntParameter("Scale option", 1);
  //snoptHandler.setParameter("Scale tolerance                0.9");
  snoptHandler.setParameter((char *) "Scale Print");
  snoptHandler.setParameter((char *) "Solution  Yes");
  //snoptHandler.setParameter("Start Objective Check at Column 1");
  //snoptHandler.setParameter("Start Constraint Check at Column 1");
  //snoptHandler.setParameter("Stop Objective Check at Column");
  //snoptHandler.setParameter("Stop Constraint Check at Column");
  //snoptHandler.setParameter("Sticky parameters               No");
  //snoptHandler.setParameter("Summary frequency               100");
  //snoptHandler.setParameter("Superbasics limit");
  //snoptHandler.setParameter("Suppress parameters");
  //snoptHandler.setParameter((char*)"System information  No");
  //snoptHandler.setParameter("Timing level                    3");
  //snoptHandler.setRealParameter("Unbounded objective value   1.0e+15");
  //snoptHandler.setParameter("Unbounded step size             1.0e+18");
  //snoptHandler.setIntParameter("Verify level", -1); //-1
  //snoptHandler.setRealParameter("Violation limit", 1e-8); //1e-8

//  if (settings_->verb_vector()[6] >= 1) // idx:6 -> opt (Optimization)
//    cout << "[opt]Set options for SNOPT.---" << endl;

}

/*****************************************************
ADGPRS, version 1.0, Copyright (c) 2010-2015 SUPRI-B
Author(s): Oleg Volkov          (ovolkov@stanford.edu)
           Vladislav Bukshtynov (bukshtu@stanford.edu)
******************************************************/

// -------------------------------------------------------------------
bool Subproblem::loadSNOPT(const string libname) {

//#ifdef NDEBUG
  if (LSL_isSNOPTLoaded()) {
    printf("\x1b[33mSnopt loaded.\n\x1b[0m");
    return true;
  }

  char buf[256];
  int rc;
  if (libname.empty()) {
    rc = LSL_loadSNOPTLib(nullptr, buf, 255);
  } else {
    rc = LSL_loadSNOPTLib(libname.c_str(), buf, 255);
  }

  if (rc) {
    string errmsg;
    errmsg = "Selected NLP solver SNOPT not available.\n"
        "Tried to obtain SNOPT from shared library \"";
    errmsg += LSL_SNOPTLibraryName();
    errmsg += "\", but the following error occured:\n";
    errmsg += buf;
    cout << errmsg << endl;
    return false;
  }
//#endif

  return true;
}

// -------------------------------------------------------------------
int SNOPTusrFG3_(integer *Status, integer *n, double x[],
                 integer *needF, integer *neF, double F[],
                 integer *needG, integer *neG, double G[],
                 char *cu, integer *lencu,
                 integer iu[], integer *leniu,
                 double ru[], integer *lenru) {


  F[0] = x[0] * x[0] + x[1] + x[1]; // objective function
  F[1] = x[0] + 2 * x[1]; // nonlinear constraint 1

  G[0] = 2 * x[0];
  G[1] = 2 * x[1];
  G[2] = 1;
  G[3] = 2;

  /*
  Eigen::MatrixXd H(2,2);
  //H.setOnes();
  H << 1, 0, 0, 1;
  Eigen::VectorXd g(2);
  g.setZero();
  double c = 0;
  int nf = *neF;
  //double x2 = x[1];
  //cout << x1 << "\t" << x2 << endl;
  //cout << "[SNOPTusrFG_] \t The x vector is: \t ";
  //for (int i = 0; i < *n; i++ ){
  //    cout << x[i] << "\t";
  //}
  //cout << endl;
  Eigen::VectorXd xvec(*n);
  for (int i = 0; i < *n; ++i){
    xvec[i] = x[i];
  }

  // -----------------------------------------------------------------
  // Calculate objective function value
  double mx = c;
  for (int i = 0; i < *n; i++) {
    mx += g[i] * x[i];
   double temp = 0;
    for (int j = 0; j < *n; j++) {
      temp += x[j] * H(i,j);
    }
    mx += temp * x[i];
  }

  F[0] = mx;

  //F[0] = c + g.transpose()*xvec + xvec.transpose()*H*xvec;
  // -----------------------------------------------------------------
  // Calculate the gradient of the objective function.
  for (int i = 0; i < *n; i++) {
    G[i] = g[i];
    for (int j = 0; j < *n; j++) {
      G[i] += H(i,j)*x[j];
    }
  }
  //Eigen::VectorXd newgrad(2);
  //newgrad = g + H*xvec;

  //G[0] = newgrad[0];G[1] = newgrad[1];

  //double rho = 30.0;
  /// Calculate constraint values.
  // The trust region constraint
  double constraint = xvec(0) + xvec(1);
  F[1] = constraint;



  //Eigen::VectorXd gradientOfC(2);
  //if ( std::abs( xvec.norm()) <= 0.00001){
  //    gradientOfC.setZero();
  //}
  //else
  //gradientOfC = xvec/(xvec.norm() + 0.00000000000000000000001);

  //G[2] = gradientOfC[0];
  //G[3] = gradientOfC[1];
  G[2] = 1;
  G[3] = 1;
*/
  /*
 for (int i = 0; i < *n; i++){
   G[*n + i] = gradientOfC[i];
 }
*/
  /*
  double h = 0.001;
  Eigen::VectorXd xper = xvec;
  xper[0] += h;
  G[2] = (xper.norm() - xvec.norm()) / h;

  xper = xvec;
  xper[1] += h;
  G[3] = (xper.norm() - xvec.norm()) / h;
*/

  // -----------------------------------------------------------------
  // Calculate gradient of constraints.

  // =================================================================
  // Computes the nonlinear objective and constraint terms for the
  // problem featured of interest. The problem is considered to be
  // written as:
  //
  //       Minimize     Fobj(x)
  //          x
  //
  //    subject to:
  //
  //        bounds      l_x <=   x  <= u_x
  //   constraints      l_F <= F(x) <= u_F
  //
  // The triples (g(k),iGfun(k),jGvar(k)), k = 1:neG, define
  // the sparsity pattern and values of the nonlinear elements
  // of the Jacobian.
  // =================================================================

  // ADGPRS LEGACY
  //  OptimizationData& optdata =  OptimizationData::reference();
  //
  //  if (( optdata.numberOfSimulations >= optdata.maxNumberOfSimulations ) &&
  //      ( optdata.maxNumberOfSimulations != 0 ))
  //  {
  //    *Status = -2;
  //    return 0;
  //  }

  // -----------------------------------------------------------------
  // number of constraints; neF is the total number of constraints
  // plus the objective
  //  int m = *neF - 1 - optdata.numberOfLinearConstraints;
  int m = *neF - 1;

  // -----------------------------------------------------------------
  // If the values for the objective are desired
  if (*needF > 0) {
    //F[0] = -(x[0] - 1.2) * (x[0] - 1.2) - (x[1] - 3.1) * (x[1] - 3.1) + constant;
    //F[3] = 0.7 * (x[0] * x[0]);// + x[1]*x[1]);

    // ADGPRS LEGACY
    // The value of the objective goes to the first entry of F
    //    if (FAILED == optdata.pOptimizationProblem->eval_f(*n, x, true, F[0]))
    //    {
    //      *Status = -1;
    //      return 0;
    //    }

    // the values of the constraints follow that of the objective
    if (m) {

      // ADGPRS LEGACY
      // optdata.pOptimizationProblem->eval_g(*n, x, false, m, &F[1]);
    }
  }

  // -----------------------------------------------------------------
  // If the values for the constraints are desired
  if (*needG > 0) {
    //G[0] = -2 * (x[0] - 1.2);
    //G[1] = -2 * (x[1] - 3.1);

    // ADGPRS LEGACY
    // We have as many derivatives as the number of the controls, n
    //    optdata.pOptimizationProblem->eval_grad_f(*n, x, false, G);

    //G[2] = 1.4 * x[0];
    //G[3] = 1.4*x[1];
    // and the derivatives of the constraints follow

    if (m) {

      // ADGPRS LEGACY
      // G[1] = 100*4*x1*x1*x1;//-4*(x2-0.7);
      // optdata.pOptimizationProblem->eval_jac_g(*n, x, false, m, *neG, 0, 0, &G[*n]);

    }

  }

  return 0;
}

// -------------------------------------------------------------------
void Subproblem::setQuadraticModel(double c,
                                   Eigen::VectorXd g,
                                   Eigen::MatrixXd H) {
  constant = c;
  int n = g.rows();
  for (int i = 0; i < n; ++i) {
    gradient[i] = g(i);
    for (int j = 0; j < n; ++j) {
      hessian[i * n + j] = H(i, j);
    }
  }
}

// -------------------------------------------------------------------
void Subproblem::setGradient(Eigen::VectorXd g) {
  int n = g.rows();
  for (int i = 0; i < n; ++i) {
    gradient[i] = g(i);
  }
}

// -------------------------------------------------------------------
void Subproblem::setHessian(Eigen::MatrixXd H) {
  int n = H.rows();
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      hessian[i * n + j] = H(i, j);
    }
  }
}

// -------------------------------------------------------------------
void Subproblem::setConstant(double c) {
  constant = c;
}

}
}