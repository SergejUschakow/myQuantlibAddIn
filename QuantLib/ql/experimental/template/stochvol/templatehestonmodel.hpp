/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2015, Sebastian Schlenkrich

*/



#ifndef quantlib_templatehestonmodels_hpp
#define quantlib_templatehestonmodels_hpp

#include <complex>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <ql/errors.hpp>
#include <ql/experimental/template/auxilliaries/gausslobatto.hpp>
#include <ql/experimental/template/auxilliaries/Complex.hpp>



#define _MIN_( a, b ) ( (a) < (b) ? (a) : (b) )
#define _MAX_( a, b ) ( (a) > (b) ? (a) : (b) )

namespace QuantLib {

    //using namespace std;
	
	// heston model with analytic vanilla pricing 
	//    dS(t) = S(t) sqrt[ v(t) ] dW(t)
    //    dv(t) = kappa [theta - v(t)] dt + sigma sqrt[ v(t) ] dZ(t)
    //    dW(t) dZ(t) = rho dt

	template <class DateType, class PassiveType, class ActiveType>
	class TemplateHestonModel {
    //typedef std::complex<Real> complex;
    typedef Cpx::Complex<ActiveType> complex;
	protected:
        ActiveType kappa_;    // mean reversion speed of volatility
        ActiveType theta_;    // mean reversion level of volatility
        ActiveType sigma_;    // volatility of volatility
        ActiveType rho_;      // correlation vol vs. underlying
        ActiveType v0_;       // initial volatility
    public:
        // constructor
        TemplateHestonModel( ActiveType kappa,
                             ActiveType theta,
                             ActiveType sigma,
                             ActiveType rho,
                             ActiveType v0 ) :
        kappa_(kappa), theta_(theta), sigma_(sigma), rho_(rho), v0_(v0) {}
                              
        // inspectors
        const ActiveType& kappa() const { return kappa_; }
        const ActiveType& theta() const { return theta_; }
        const ActiveType& sigma() const { return sigma_; }
        const ActiveType& rho()   const { return rho_;   }
        const ActiveType& v0()    const { return v0_;    }
        // maths
        bool fellerConstraint() {
            return (sigma >= 0.0 && sigma*sigma < 2.0*kappa*theta);
        }
        // undiscounted expectation of vanilla payoff
        ActiveType vanillaOption(const PassiveType forwardPrice,
                                 const PassiveType strikePrice,
                                 const DateType    term,
                                 const int         callOrPut,
                                 const PassiveType accuracy,
                                 const size_t      maxEvaluations) {
            const ActiveType c_inf = _MIN_(10.0, _MAX_(0.0001, sqrt(1.0-rho_*rho_)/sigma_)) * (v0_ + kappa_*theta_*term);
            TemplateAuxilliaries::GaussLobatto<ActiveType> integrator(maxEvaluations, accuracy);
            IntegrandGatheral gatheral1( *this, forwardPrice, strikePrice, term, 1 );
            IntegrandGatheral gatheral2( *this, forwardPrice, strikePrice, term, 2 );
            IntegrandTransformation transf1( c_inf, gatheral1 );
            IntegrandTransformation transf2( c_inf, gatheral2 );
            const ActiveType p1 = integrator.integrate( transf1, 0, 1) / M_PI; 
            const ActiveType p2 = integrator.integrate( transf2, 0, 1) / M_PI; 
            switch (callOrPut) {
                case +1:  // Call
                    return forwardPrice*(p1+0.5) - strikePrice*(p2+0.5);
                    break;
                case -1:  // Put
                    return forwardPrice*(p1-0.5) - strikePrice*(p2-0.5);
                    break;
                default:
                    QL_FAIL("unknown option type");
            }
        }

        // f_1/2( u(x) ) / ( x c_inf ), u(x) = -ln(x) / c_inf
        class IntegrandTransformation  {
        protected:
            ActiveType                                c_inf_;
            boost::function<ActiveType (ActiveType)>  f_12_;
        public:
            IntegrandTransformation(  const ActiveType& c_inf, const boost::function<ActiveType (ActiveType)>& f_12)
                : f_12_(f_12), c_inf_(c_inf) { }
            ActiveType operator()(ActiveType x) const {
                if (x * c_inf_ < QL_EPSILON) return 0;
                else                         return f_12_( -log(x) / c_inf_ ) / x / c_inf_;
            }

        };

        // key issue of Heston model, implements f_1/2 (phi) according to Gatherals approach
        class IntegrandGatheral {
        protected:
            Size j_;                                 // evaluate f_1 or f_2
            const ActiveType kappa_, theta_, sigma_, v0_;  // copy of model parameters
            // helper variables
            const DateType   term_;
            const ActiveType x_, sx_, dd_;
            const ActiveType sigma2_, rsigma_;
            const ActiveType t0_;
        public:
            // constructor
            IntegrandGatheral( 
                const TemplateHestonModel<DateType,PassiveType,ActiveType> &model,
                const ActiveType forward,     // underlying initial state
                const ActiveType strike,      // vanilla option strike
                const DateType   term,        // time to maturity
                const Size       j            // f1 or f2
                ) :
                kappa_(model.kappa()), theta_(model.theta()), sigma_(model.sigma()), v0_(model.v0()),
                term_(term),
                j_(j),
                x_(log(forward)),
                sx_(log(strike)),
                dd_(x_),
                sigma2_(sigma_*sigma_),
                rsigma_(model.rho()*sigma_),
                t0_(kappa_ - ((j== 1)? rsigma_ : 0)) {  }
            // ...
            ActiveType operator()(ActiveType phi) const {
                const ActiveType rpsig(rsigma_*phi);
                const complex t1 = t0_+complex(0, -rpsig);
                const complex d  = sqrt(t1*t1 - sigma2_*phi*complex(-phi, (j_== 1)? 1 : -1));
                const complex ex = exp(-d*ActiveType(term_));
                const complex addOnTerm =  0.0;
                if (phi != 0.0) {
                    if (sigma_ > 1e-5) {
                        const complex p = (t1-d)/(t1+d);
                        const complex g = log((1.0 - p*ex)/(1.0 - p));
                        return exp(v0_*(t1-d)*(1.0-ex)/(sigma2_*(1.0-ex*p))
                                    + (kappa_*theta_)/sigma2_*((t1-d)*term_-2.0*g)
                                    + complex(0.0, phi*(dd_-sx_))
                                    + addOnTerm
                                  ).imag()/phi;
                    }
                    else {
                        const complex td = phi/(2.0*t1)*complex(-phi, (j_== 1)? 1 : -1);
                        const complex p  = td*sigma2_/(t1+d);
                        const complex g  = p*(1.0-ex);
                        return exp(v0_*td*(1.0-ex)/(1.0-p*ex)
                                         + (kappa_*theta_)*(td*term_-2.0*g/sigma2_)
                                         + complex(0.0, phi*(dd_-sx_))
                                         + addOnTerm
                                       ).imag()/phi;
                    }
                }
                else {
                    // use l'Hospital's rule to get lim_{phi->0}
                    if (j_ == 1) {
                        const ActiveType kmr = rsigma_-kappa_;
                        if (fabs(kmr) > 1e-7) {
                            return dd_-sx_ + (exp(kmr*term_)*kappa_*theta_
                                   -kappa_*theta_*(kmr*term_+1.0) ) / (2*kmr*kmr)
                                   - v0_*(1.0-exp(kmr*term_)) / (2.0*kmr);
                        }
                        else
                            // \kappa = \rho * \sigma
                            return dd_-sx_ + 0.25*kappa_*theta_*term_*term_ + 0.5*v0_*term_;
                    }
                    else {
                        return dd_-sx_ - (exp(-kappa_*term_)*kappa_*theta_
                               + kappa_*theta_*(kappa_*term_-1.0))/(2*kappa_*kappa_)
                               - v0_*(1.0-exp(-kappa_*term_))/(2*kappa_);
                    }
                }        
                return 0;
            }  // operator()

        }; // class IntegrandGatheral

	};  // TemplateHestonModel        

	// general stochastic volatility model with constant parameters and analytic vanilla pricing formula
	//
	//    dS(t) = lambda [ b S(T) + (1-b) L ] sqrt[z(t)] dW(t)
	//    dz(t) = theta [ m - z(t) ] dt + eta sqrt[z(t)] dZ(t)
	//    dW(t) dZ(t) = rho dt
	//
	template <class DateType, class PassiveType, class ActiveType>
	class TemplateStochVolModel {
	protected:
		boost::shared_ptr< TemplateHestonModel<DateType,PassiveType,ActiveType> > hestonModel_;
		ActiveType                                                                shift_;
	public:
		TemplateStochVolModel ( const ActiveType   lambda,
			                    const ActiveType   b,
								const ActiveType   L,
								const ActiveType   theta,
								const ActiveType   m,
								const ActiveType   eta,
								const ActiveType   z0,
								const ActiveType   rho )
		: hestonModel_(new TemplateHestonModel<DateType,PassiveType,ActiveType>(
		    // state transformations ~S(t) = S(t) + (1-b)/b L, v(t) = z(t) lambda^2 b^2
			theta,                // kappa
			m*lambda*lambda*b*b,  // theta
			eta*lambda*b,         // sigma
			rho,                  // rho
			z0*lambda*lambda*b*b  // v0
			) ), shift_( (1.0-b)/b*L ) {}

        // undiscounted expectation of vanilla payoff
        ActiveType vanillaOption(const PassiveType forwardPrice,
                                 const PassiveType strikePrice,
                                 const DateType    term,
                                 const int         callOrPut,
                                 const PassiveType accuracy,
                                 const size_t      maxEvaluations) {
			return hestonModel_->vanillaOption( forwardPrice+shift_, strikePrice+shift_, term, callOrPut, accuracy, maxEvaluations );
		}


	};


}

#undef _MIN_
#undef _MAX_

#endif  /* ifndef quantlib_hestonmodels_hpp */
