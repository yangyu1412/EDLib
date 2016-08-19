//
// Created by iskakoff on 01/08/16.
//

#ifndef HUBBARD_GREENSFUNCTION_H
#define HUBBARD_GREENSFUNCTION_H


#include <iomanip>
#include "Lanczos.h"
template<typename precision, class Hamiltonian>
class GreensFunction: public Lanczos<precision, Hamiltonian> {
  using Lanczos<precision, Hamiltonian>::hamiltonian;
  using Lanczos<precision, Hamiltonian>::lanczos;
  using Lanczos<precision, Hamiltonian>::omega;
  using Lanczos<precision, Hamiltonian>::computefrac;
public:
  GreensFunction(EDParams& p, Hamiltonian& h) : Lanczos<precision, Hamiltonian>(p, h), _model(h.model()),
                                                    gf(p["NSPINS"], std::vector<alps::gf::omega_gf>(p["NSITES"], alps::gf::omega_gf(Lanczos<precision, Hamiltonian>::omega() ) ) ),
                                                    _cutoff(p["lanc.BOLTZMANN_CUTOFF"]){
  }

  void compute() {
    auto & ground_state = hamiltonian().eigenpairs()[0];
    double Z = 0.0;
    for(auto & eigenpair : hamiltonian().eigenpairs()) {
      Z += std::exp(-(eigenpair.eigenvalue() - ground_state.eigenvalue())*omega().beta());
    }
    for(auto & eigenpair : hamiltonian().eigenpairs()) {
      double boltzmann_f = std::exp(-(eigenpair.eigenvalue() - ground_state.eigenvalue()) * omega().beta());
      if(boltzmann_f < _cutoff) {
//        std::cout<<"Skipped by Boltzmann factor."<<std::endl;
        continue;
      }
      std::cout<<"Compute Green's function contribution for eigenvalue E="<<eigenpair.eigenvalue()<<" with Boltzmann factor"<<boltzmann_f<<"; for sector"<<eigenpair.sector()<<std::endl;
      for(int i = 0; i<1/*_model.orbitals()*/; ++i) {
        for(int is = 0; is< _model.spins() ; ++is) {
          std::vector<precision> outvec(eigenpair.sector().size(), precision(0.0));
          double expectation_value = 0;
          _model.symmetry().set_sector(eigenpair.sector());
          if(_model.create_particle(i, is, eigenpair.eigenvector(), outvec, expectation_value)){
            int nlanc = lanczos(outvec);
            std::cout<<"orbital: "<<i<<"   spin: "<<(is == 0 ? "up" :"down")<<" <n|a*a|m>="<<expectation_value<<std::endl;
            computefrac(expectation_value, eigenpair.eigenvalue(), ground_state.eigenvalue(), nlanc, 1, gf[is][i]);
          }
          _model.symmetry().set_sector(eigenpair.sector());
          if(_model.annihilate_particle(i, is, eigenpair.eigenvector(), outvec, expectation_value)){
            int nlanc = lanczos(outvec);
            std::cout<<"orbital: "<<i<<"   spin: "<<(is == 0 ? "up" :"down")<<" <n|a*a|m>="<<expectation_value<<std::endl;
            computefrac(expectation_value, eigenpair.eigenvalue(), ground_state.eigenvalue(), nlanc, -1, gf[is][i]);
          }
        }
      }
    }
    for(int i = 0; i<1/*_model.orbitals()*/; ++i) {
      for (int is = 0; is < _model.spins(); ++is) {
        gf[is][i] /= Z;
        std::ostringstream Gomega_name;
        Gomega_name<<"G_omega"<<"_"<<(is ? "down": "up")<<"_"<<i;
        std::ofstream G_omega_file(Gomega_name.str().c_str());
        G_omega_file << std::setprecision(14) << gf[is][i];
        Gomega_name<<".h5";
        alps::hdf5::archive ar(Gomega_name.str().c_str(), alps::hdf5::archive::WRITE);
        gf[is][i].save(ar, "/G_omega");
        G_omega_file.close();
        ar.close();
      }
    }
    std::cout<<"Statsum: "<<Z<<std::endl;
  }
private:
  std::vector< std::vector<alps::gf::omega_gf> > gf;
  typename Hamiltonian::ModelType& _model;
  double _cutoff;
};


#endif //HUBBARD_GREENSFUNCTION_H
