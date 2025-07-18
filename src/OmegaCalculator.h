#ifndef OMEGA_CALCULATOR_H
#define OMEGA_CALCULATOR_H 

/* #include"TransportCrossSection.h" */
#include"DataLoader.h"

// forward declarations 
class Species ; 
class ChargedSpecies ;
class InteractionInterface ;
class CsCalculator ; 
class CsHolder ; 
class MultiCs ; 
class Potential ;
class TcsInterface ;  

/** @brief Interface class for algorithms to compute on Collision Integrals.
 * A pointer to this base class is possessed by a CollisionIntegral object which 
 * initialize the method polimorphically */
class OmegaCalculator {

    protected:

    /// @brief "SpeciesFormula_SpeciesFormula"
    std::string interactionName ; 
    Species* sp1 ; 
    Species* sp2 ; 


    /// @brief Base constructor of CsCalculator classes assign species and interactionName.
    /// @param sp1 
    /// @param sp2 
    OmegaCalculator(Species*sp1,Species*sp2) ;
    
    /// @brief Base constructor of CsCalculator classes assign interactionName.
    OmegaCalculator(InteractionInterface* i) ;

    public:

    /** @brief Interface function to compute the collision integral
     * @param l-th momentum of the TCS
     * @param s-th order of the Sonine polynomials 
     * @param T temperature of collision [K°] 
     * @param lambda debye length [m]
     * @param TcS Pointer to the TransportCrossSection interface to access TCS methods
     * without occurring in circular dependance 
     * @return double value of the collision integral funtion of l,s,T and lambda  */
    virtual double Compute( int l, int s, double T, double lambda, CsCalculator* TcS ) = 0;
    
    /// @brief info on the algorithm for the specific interaction.
    virtual void info() = 0 ;
    
    virtual ~OmegaCalculator() = default;

};

/** @brief class for the Loading of the collision integrals through raw .txt files.
 * further developments will involve more flexible formats to load on to */
class LoaderOmega : public OmegaCalculator, public DataLoader {


    std::vector<double> temperatures;
    std::vector<std::vector<double>> qColumns;

    void Init() override { if(!loaded) LoadData("Collision_Integrals",interactionName); }

    ~LoaderOmega(){};

    public:

    /** @brief construct a Loader object searching for the raw file named as the interaction
     * (convention for this program for raw computed 4-th order 
     * collision integrals .txt datafiles) and storing data on this class members */
    LoaderOmega( InteractionInterface* i ) ;
    LoaderOmega( const std::string& prefix , InteractionInterface* i ) ;

    /** @brief compute Omega^(l,s)(T) interpolating with a positive cubic Spline the loaded data 
     * @see OmegaCalculator for info on the parameters */
    double Compute(int l, int s, double temperatura, double lambda, CsCalculator* TcS) override ;

    /// @brief info on the object.
    void info() override { std::cout << "Raw loader"; }

    /// @brief string filename to load
    std::string BuildFileName(const std::string& name) override {
        return "Q4th_"+customPrefix+name+".txt";
    }

    void ParseFile(std::ifstream& file) override;
    
};


/** @brief class for the calculation of ChargedSpecie-ChargedSpecie (Coulomb)
 * collision integral. */
class CoulombOmega : public OmegaCalculator {

    ChargedSpecies* sp1;
    ChargedSpecies* sp2;

    /** @brief Matrix that stores computed Coulomb collision integrals on adimensional 
     * temperatures (the first row) for attractive (even rows) and repulsive (odd rows) 
     * shielded coulomb potential, (l,s) combinations necessary for the 4-th order approximation
     * to transport properties. @see CoulombOmega::CoulombOmega for more. */
    std::vector<std::vector<double>> qcTxt ;

    public:

    /// @brief Constructor to CoulombOmega class, initialize Qctxt hard-coded data.
    CoulombOmega(ChargedSpecies* s1, ChargedSpecies* s2) ;

    /// @brief Print info when called.    
    virtual void info() override {std::cout<<"Coulomb collision"; } ;

    /** @brief compute Omega^(l,s)(T) for Coulomb collision, 
     * @see OmegaCalculator for info on the parameters */
    double Compute( int l, int s, double T, double Lam, CsCalculator* TcS ) override ;

};

/** @brief class for the calculation of Non-coulomb (neutral-neutral, neutral-ion) interactions
 * integrating computed TransportCrossSection. */
class NonCoulombOmega : public OmegaCalculator {

    protected:

    /// @brief 32th order Laguerre polynomial nodes to integrate Transport Cross Section. 
    std::vector<double> x ;
   
    /// @brief 32th order Laguerre polynomials related weights.
    std::vector<double> w ;

    double ComputeModuleOmega ( int l, int s, double T, double Lam, CsHolder* TcS ) ;

    double IntegrateOmega ( int l, int s, double T, double Lam, CsCalculator* TcS ) ; 

    public:

    /// @brief Initialize members x and w hard-coded data and interactionName.
    NonCoulombOmega( InteractionInterface* i ) ;
    
    /** @brief Initialize members x and w hard-coded data and interactionName.
     ** substitute calculator with ChiIntegrator and gives it Potential */
    NonCoulombOmega( InteractionInterface* i , TcsInterface* t, Potential* pot ) ;

    /// @brief Print info when called.    
    virtual void info() override {std::cout<<"Non-coulomb coll." ; } ;

    /** @brief compute Omega^(l,s)(T) for NonCoulomb collision, 
     * @see OmegaCalculator for info on the parameters */
    double Compute ( int l, int s, double T, double Lam, CsCalculator* TcS ) override ;

};

class MultiPotOmega : public NonCoulombOmega {

    /// @brief User-given degeneracies holder
    std::vector<double> degeneracies ;

    double MultiCompute ( int l, int s, double T, double Lam, MultiCs* p ) ;

    public:

    MultiPotOmega( InteractionInterface* i, TcsInterface* TcS, 
        std::initializer_list<Potential*> phis, std::initializer_list<double> gs ) ;

    MultiPotOmega ( InteractionInterface* i, TcsInterface* TcS, std::initializer_list<double> gs ) ;

    /// @brief Print info when called 
    virtual void info() override {std::cout<<"  Multi Potential" ; } ;

    /** @brief Compute Omega^(l,s)(T) for NonCoulomb collisions involving different states,
     ** of the involved species in a multi potential approach to excited states.
     ** @see OmegaCalculator for info on the parameters */
    double Compute ( int l, int s, double T, double Lam, CsCalculator* TcS ) override ;


};

/** @brief class to account forcharge exchange phenomena in the inelastic 
 * contribution to collision integrals */
class ChargeExchangeOmega : public OmegaCalculator {

    /** @brief Inner elastic collision calculator copied from the Collision Integral
     * object during contruction. */
    OmegaCalculator* omegaEl ; 
    
    double A,B;

    public: 
    
    ChargeExchangeOmega( InteractionInterface* i , OmegaCalculator* OMel, double A , double B ) ;

    /** @brief Compute the elastic part with the previous calculator and the inelastic
     * part with the analytical formula 12 of Devoto R S 1967 Phys. Fluids 10 354. 
     * for the charge exchange collision integral. 
     * @return the square modulus of Qel and Qe for odd l values . 
     * @see OmegaCalculator for info on the parameters */
    double Compute ( int l, int s, double temperatura, double lambda, CsCalculator* TcS ) override ;

    /// @brief Print info when called 
    void info() override { omegaEl->info(); std::cout << " & Charge Exchange " ; }

};

// _______________________________ Implementation _____________________________

#endif