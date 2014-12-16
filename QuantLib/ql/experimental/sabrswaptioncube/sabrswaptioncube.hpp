/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2010 Sebastian Schlenkrich
*/

/*! \file bondoptionengine.hpp
    \brief engine for bond options with Hull White model
*/

#ifndef quantlib_sabrswaptioncube_hpp
#define quantlib_sabrswaptioncube_hpp


#include <ql/termstructures/volatility/swaption/swaptionvolstructure.hpp>
#include <ql/termstructures/volatility/sabrsmilesection.hpp>
#include <ql/math/interpolations/flatextrapolation2d.hpp>
#include <ql/math/interpolations/bilinearinterpolation.hpp>



namespace QuantLib {

    // bilinear interpolation of SABR parameters
    class SabrSwaptionCube : public SwaptionVolatilityStructure {
    private:
        std::vector< Time > optionTimes_, swapTimes_;
        Matrix alpha_, beta_, rho_, nu_, fwd_;
        boost::shared_ptr<Interpolation2D> alphaInterp_, betaInterp_, rhoInterp_, nuInterp_, fwdInterp_;
        Period maxSwapTenor_;
        Date referenceDate_;
        // we interpolate forward swap rates as well
        // add default swaption propertes for more accurate forward valuation
    public:

        SabrSwaptionCube ( const std::vector<Time>&                   optionTimes,
                           const std::vector<Time>&                   swapTimes,
                           const std::vector< std::vector< Real > >&  alpha,
                           const std::vector< std::vector< Real > >&  beta,
                           const std::vector< std::vector< Real > >&  rho,
                           const std::vector< std::vector< Real > >&  nu,
                           const std::vector< std::vector< Real > >&  fwd,
                           BusinessDayConvention                      bdc,
                           const DayCounter&                          dc = DayCounter());

        // implement bilinear parameter interpolation
        virtual boost::shared_ptr<SmileSection> smileSectionImpl(
                           Time optionTime,
                           Time swapLength) const;

        virtual Volatility volatilityImpl(Time optionTime,
                           Time swapLength,
                           Rate strike) const { return smileSectionImpl(optionTime, swapLength)->volatility(strike); }

        virtual const Date& referenceDate() const { return referenceDate_; }
        virtual const Period& maxSwapTenor() const { return maxSwapTenor_; }
        virtual Date maxDate() const { return Date(1, December, 2100); }
        virtual Rate minStrike() const { return 0; }
        virtual Rate maxStrike() const { return 100; }

    };


}

#endif