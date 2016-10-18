//
// Created by iskakoff on 20/07/16.
//

#ifndef HUBBARD_CRSSTORAGE_H
#define HUBBARD_CRSSTORAGE_H


#include <vector>
#include <iomanip>
#include "fortranbinding.h"
#include "Storage.h"

namespace EDLib {
  namespace Storage {
    template<typename prec, class Model>
    class CRSStorage : public Storage < prec > {
      using Storage < prec >::n;
      using Storage < prec >::ntot;
    public:
#ifdef USE_MPI
      CRSStorage(alps::params &p, Model &s, alps::mpi::communicator & comm) : Storage < prec >(p, comm),
#else
      CRSStorage(EDParams &p, Model &s) : Storage < prec >(p),
#endif
                                          _vind(0), _model(s) {
        _max_size = p["storage.MAX_SIZE"];
        _max_dim = p["storage.MAX_DIM"];
        // init what you need from parameters
      };

      void reset(size_t sector_size) {
        if (sector_size > _max_dim) {
          std::stringstream s;
          s << "Current sector request more memory than allocated. Increase MAX_DIM parameter. Requested " << sector_size << ", allocated " << _max_dim << ".";
          throw std::runtime_error(s.str().c_str());
        }
        _vind = 0;
        row_ptr.assign(_max_dim + 1, 0);
        col_ind.assign(_max_size, 0);
        values.assign(_max_size, prec(0.0));
        n() = 0;
      }

      void inline addDiagonal(const int &i, prec v) {
        row_ptr[i] = _vind;
        col_ind[_vind] = i;
        values[_vind] = v;
        ++_vind;
        ++n();
        ++ntot();
      }

      /**
       * Add off-diagonal H(i,j) element
       */
      void inline addElement(const int &i, int j, prec t, int sign) {
        bool hasstate = false;
        size_t foundstate = 0;
        // check that there is no any data on the k state
        for (size_t iii = row_ptr[i]; iii < _vind; ++iii) {
          if (col_ind[iii] == j) {
            hasstate = true;
            foundstate = iii;
          }
        }
        // In case of multi-orbital Coulomb interaction we can have contribution from different Coulomb interactions
        if(hasstate) {
          values[foundstate] += sign * t;
        } else {
          // create new element in CRS arrays
          col_ind[_vind] = j;
          values[_vind] = sign * t;
          ++_vind;
        }
        if (_vind > _max_size) {
          std::stringstream s;
          s << "Current sector request more memory than allocated. Increase MAX_SIZE parameter. Requested " << _vind << ", allocated " << _max_size << ".";
          throw std::runtime_error(s.str().c_str());
        }
      }

      /**
       * Simple Compressed-Row-Storage Matrix-Vector product
       */
      virtual void av(prec *v, prec *w, int n, bool clear = true) {
        for (int i = 0; i < n; ++i) {
          w[i] = clear ? 0.0 : w[i];
          for (int j = row_ptr[i]; j < row_ptr[i + 1]; ++j) {
            w[i] = w[i] + values[j] * v[col_ind[j]];
          }
        }
      }

      void fill() {
        _model.symmetry().init();
        reset(_model.symmetry().sector().size());
        int i = 0;
        long long k = 0;
        int isign = 0;
        while (_model.symmetry().next_state()) {
          long long nst = _model.symmetry().state();
          // Compute diagonal element for current i state

          addDiagonal(i, _model.diagonal(nst));
          // non-diagonal terms calculation
          off_diagonal<decltype(_model.T_states())>(nst, i, _model.T_states());
          off_diagonal<decltype(_model.V_states())>(nst, i, _model.V_states());
          i++;
        }
        endMatrix();
      }

      template<typename T_states>
      inline void off_diagonal(long long nst, int i, T_states states) {
        long long k = 0;
        int isign = 0;
        for (int kkk = 0; kkk < states.size(); ++kkk) {
          if (_model.valid(states[kkk], nst)) {
            _model.set(states[kkk], nst, k, isign);
            int k_index = _model.symmetry().index(k);
            addElement(i, k_index, states[kkk].value(), isign);
          }
        }
      };

      void endMatrix() {
        row_ptr[n()] = _vind;
      }

      void print() {
        std::cout << std::setprecision(2) << std::fixed;
        std::cout << "{";
        for (int i = 0; i < n(); ++i) {
          std::cout << "{";
          for (int j = 0; j < n(); ++j) {
            bool f = true;
            for (int k = row_ptr[i]; k < row_ptr[i + 1]; ++k) {
              if ((col_ind[k]) == j) {
                std::cout << std::setw(6) << values[k] << (j == n() - 1 ? "" : ", ");
                f = false;
              } /*else {
            std::cout<<"0.0 ";
          }*/
            }
            if (f) {
              std::cout << std::setw(6) << 0.0 << (j == n() - 1 ? "" : ", ");
            }
          }
          std::cout << "}" << (i == n() - 1 ? "" : ", \n");
        }
        std::cout << "}" << std::endl;
      }

      virtual void zero_eigenapair() {
        Storage < prec >::eigenvalues().resize(1);
        Storage < prec >::eigenvalues()[0] = values[0];
        Storage < prec >::eigenvectors().assign(1, std::vector < prec >(1, prec(1.0)));
      }
      size_t vector_size(typename Model::Sector sector) {
        return sector.size();
      }
      prec vv(const std::vector<prec> & v, const std::vector<prec> & w) {
        prec alf = prec(0.0);
        for (int k = 0; k < v.size(); ++k) {
          alf += w[k] * v[k];
        }
        return alf;
      }

    private:
      std::vector < prec > values;
      std::vector < int > row_ptr;
      std::vector < int > col_ind;
      size_t _max_size;
      size_t _max_dim;


      size_t _vind;

      Model &_model;
    };

  }
}
#endif //HUBBARD_CRSSTORAGE_H
