#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "config.h"
#include "boost/filesystem.hpp"
#include <vector>
#include <string>


enum TIME_SCHEMES
{
  EXPLICIT,      // explicit scheme
  CRANK_NICOLSON // sort of implicit scheme
};



class Parameters
{
public:
            /**
             * Dimension of the problem.
             * The whole program is designed to solve the problem of specific dimension.
             */
  static const int DIM = 2;

            /**
             * The number of the domain that characterizes the inclusions
             */
  static const int INCL_DOMAIN = 11;

            /**
             * A time scheme we use to approximate the second time derivative of the wave equation.
             * It can be explicit or Crank-Nicolson
             */
  int TIME_SCHEME;

            /**
             * The limits of the 2D computational domain.
             * The points (X_BEG, Y_BEG) and (X_END, Y_END) are the mesh nodes only if the domain is rectangular.
             * In case of domain with curvilinear boundaries, these limits just show the maximal possible range of coordinates
             */
  double X_BEG, X_END, Y_BEG, Y_END;

            /**
             * The number of fine rectangular elements in x- and y-directions
             */
  unsigned int N_FINE_X, N_FINE_Y;

            /**
             * The time of the begin of the simulation (usually it's 0), and the time of its end
             */
  double TIME_BEG, TIME_END;

            /**
             * The time step
             */
  double TIME_STEP;

            /**
             * The number of the time steps
             */
  unsigned int N_TIME_STEPS;

            /**
             * The order of the finite element basis functions
             */
  int FE_ORDER;
  //int QUAD_ORDER; // the order of quadrature formula

            /**
             * The frequency of the Ricket wavelet, that is used as a source of seismic waves
             */
  double SOURCE_FREQUENCY;

            /**
             * The support of the source function
             */
  double SOURCE_SUPPORT;

            /**
             * x-coordinate of the center of the source
             */
  double SOURCE_CENTER_X;

            /**
             * y-coordinate of the center of the source
             */
  double SOURCE_CENTER_Y;

            /**
             * The number of subdomains with different physical properties in the model
             */
  unsigned int N_SUBDOMAINS;

            /**
             * The names of the files where the distribution of
             * the coefficient alpha from the equation
             * alpha * u'' - div( beta grad u ) = f
             * is represented.
             * They can be empty strings what means that coefficients alpha are homogeneous
             * in the domains. In this case COEF_A_VALUES is used.
             */
  std::vector<std::string> COEF_A_FILES;

            /**
             * It is the same as COEF_A_FILES but for the coefficient beta
             */
  std::vector<std::string> COEF_B_FILES;

            /**
             * The values of the homogeneous alpha coefficients from the equation
             * alpha * u'' - div( beta grad u ) = f
             */
  std::vector<double> COEF_A_VALUES;

            /**
             * The values of the homogeneous beta coefficients from the equation
             * alpha * u'' - div( beta grad u ) = f
             */
  std::vector<double> COEF_B_VALUES;

            /**
             * Distribution of the layers is described in the file
             */
  std::string LAYERS_FILE;
  std::string LAYERS_DIR;
  bool USE_LAYERS_FILE;
  bool CREATE_BIN_LAYERS_FILE;
  std::string WHAT_BIN_LAYERS_FILE;
  //bool CREATE_AVE_LAYERS_FILE;
  double H_BIN_LAYER_PERCENT;
  std::string LAYERS_FILE_SUFFIX;
  bool USE_AVERAGED;

            /**
             * Whether we need to save a coefficients distribution for further usage in other programs or in this one
             */
  bool SAVE_COEF_PER_CELL;
  bool SAVE_COEF_PER_VERT;

            /**
             * Is there a file with saved coefficients distribution, or not
             */
  bool COEF_SAVED_PER_VERT;

            /**
             * A name of file where a coefficients distribution will be (or has been) kept
             */
  std::string COEF_FILE;

            /**
             * A directory where files with coefficients distributions are kept
             */
  std::string COEF_DIR;

//            /**
//             * The layers are distributed in parallel to each other, but probably not horizontally.
//             * This angle (in degrees, not radians) describes the slope of the layers.
//             * The angle is counted between horizontal axis (X) in 2D plan.
//             * That means horizontally distributed layers have angle 0.
//             */
//  double LAYERS_ANGLE;

            /**
             * The path to the directory where the meshes (.msh-files) are
             */
  std::string MESH_DIR;

            /**
             * The path to the directory where the geometry file (.geo-files) are
             */
  std::string GEO_DIR;

            /**
             * The path to the directory where all the results of the simulation will be kept
             */
  std::string RES_DIR;

            /**
             * The path to the directory where all results of all simulations will be kept.
             * This is a top level directory, where other directories RES_DIR are kept.
             */
  std::string RES_TOP_DIR;

            /**
             * The name of the directory where the .vtu files with the results on some time steps will be kept.
             * This is the subdirectory of the RES_DIR directory
             */
  std::string VTU_DIR;

            /**
             * The name of the directory where the .sol files with the solutions on some time steps will be kept.
             * This is the subdirectory of the RES_DIR directory
             */
  std::string SOL_DIR;

            /**
             * The name of the file where the time of calculations will be kept.
             * This file is in the RES_DIR directory
             */
  std::string TIME_FILE;

            /**
             * The name of the file where the information about simulation will be kept.
             * This file is in the RES_DIR directory
             */
  std::string INFO_FILE;

            /**
             * The name of the file with a mesh.
             * This mesh file should be in the MESH_DIR directory
             */
  std::string MESH_FILE;

            /**
             * Whether we need to print .vtu files or not
             */
  bool PRINT_VTU;

            /**
             * Whether we need to save .dat files or not
             */
  bool SAVE_SOL;

            /**
             * Whether we need to print some info on the screen during the calculations
             */
  bool PRINT_INFO;

            /**
             * Do we need to include coefficients distribution in the file with results
             */
  bool EXPORT_COEFFICIENTS;

            /**
             * If we need to print .vtu files we can do that not for each time step, but for every (VTU_STEP)-th step
             */
  unsigned int VTU_STEP;

            /**
             * If we need to save .dat files with solutions we can do that not for each time step, but for every (SOL_STEP)-th step
             */
  unsigned int SOL_STEP;

            /**
             * Constructor
             * @param argc - the number of command line arguments (+1 - the first argument is the name of the executable by default)
             * @param argv - the command line arguments themselves
             */
  Parameters(int argc = 0, char **argv = 0); // constructor

            /**
             * Destructor
             */
  ~Parameters();

            /**
             * Get the string with readable represantation of all (probably) parameters of the problem.
             * This string can be then output in file or standard output stream.
             */
  std::string print() const;

            /**
             * To make some useful things for calculations.
             * For example, create the structure of output directories, files, etc.
             * This procedure was separated from Parameter constructor to
             * avoid time-consuming operations with filesystem in default parameters objects
             * (that are used, for example, in testing procedures).
             * This function has to be called before real work!
             */
  void establish_environment();


private: //======================= PRIVATE =========================
            /**
             * We limit the number of subdomains with different physical properties
             */
  static const unsigned int _N_SUBDOMAINS_LIMIT = 100;

            /**
             * Copy constructor. It's private to protect copying parameters, because it would be weird.
             * That's also useful to prevent copying objects of the Parameter class as function argument.
             * Therefore we always have to sent parameter object as a const reference or pointer
             */
  Parameters(const Parameters&);

            /**
             * Operator of copy assignment is private for the same reason as the copy constructor
             */
  const Parameters& operator =(const Parameters&);

            /**
             * Initialize default parameters
             */
  void default_parameters();

            /**
             * Read parameters from the command line
             * @param argc - the number of the command line arguments
             * @param argv - the command line arguments
             */
  void read_from_command_line(int argc, char **argv);

            /**
             * Generate the paths to some files and directories.
             * Some of the paths are based on the parameters, therefore
             * this function should be invoked after establishing all parameters
             */
  void generate_paths();

            /**
             * Check that the directories we are going to use for output data exist and empty
             */
  void check_clean_dirs() const;
};


#endif // PARAMETERS_H
