/* -------------------------------------------------------------------------- *
 * OpenSim Muscollo: TropterProblem.cpp                                       *
 * -------------------------------------------------------------------------- *
 * Copyright (c) 2018 Stanford University and the Authors                     *
 *                                                                            *
 * Author(s): Christopher Dembia                                              *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0          *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */
#include "TropterProblem.h"

using namespace OpenSim;

template <typename T>
template <typename MucoIterateType, typename tropIterateType>
MucoIterateType MucoTropterSolver::TropterProblemBase<T>::
convertIterateTropterToMuco(const tropIterateType& tropSol) const {
    const auto& tropTime = tropSol.time;
    SimTK::Vector time((int)tropTime.size(), tropTime.data());
    const auto& state_names = tropSol.state_names;
    const auto& control_names = tropSol.control_names;

    const int& numMultipliers =
            this->m_mpSum /* + this->m_mvSum + this->m_maSum*/;
    std::vector<std::string> multiplier_names(numMultipliers);
    std::copy_n(tropSol.adjunct_names.begin(), numMultipliers,
            multiplier_names.begin());

    const int numDerivatives =
            (int)tropSol.adjunct_names.size() - numMultipliers;
    assert(numDerivatives >= 0);
    std::vector<std::string> derivative_names(numDerivatives);
    std::copy_n(tropSol.adjunct_names.begin() + numMultipliers, numDerivatives,
            derivative_names.begin());

    const auto& parameter_names = tropSol.parameter_names;

    int numTimes = (int)time.size();
    int numStates = (int)state_names.size();
    // Create and populate states matrix.
    SimTK::Matrix states;
    if (numStates) {
        states.resize(numTimes, numStates);
        for (int itime = 0; itime < numTimes; ++itime) {
            for (int istate = 0; istate < numStates; ++istate) {
                states(itime, istate) = tropSol.states(istate, itime);
            }
        }
    }
    int numControls = (int)control_names.size();
    // Instantiating a SimTK::Matrix with a zero row or column does not create
    // an empty matrix. For example,
    //      SimTK::Matrix controls(5, 0);
    // will create a matrix with five empty rows. So, for variables other than
    // states, only allocate memory if necessary. Otherwise, return an empty
    // matrix. This will prevent weird comparison errors between two iterates
    // that should be equal but have slightly different "empty" values.
    SimTK::Matrix controls;
    if (numControls) {
        controls.resize(numTimes, numControls);
        for (int itime = 0; itime < numTimes; ++itime) {
            for (int icontrol = 0; icontrol < numControls; ++icontrol) {
                controls(itime, icontrol) = tropSol.controls(icontrol, itime);
            }
        }
    }
    SimTK::Matrix multipliers;
    if (numMultipliers) {
        multipliers.resize(numTimes, numMultipliers);
        for (int itime = 0; itime < numTimes; ++itime) {
            for (int imultiplier = 0; imultiplier < numMultipliers;
                    ++imultiplier) {
                multipliers(itime, imultiplier) =
                        tropSol.adjuncts(imultiplier, itime);
            }
        }
    }
    SimTK::Matrix derivatives;
    if (numDerivatives) {
        derivatives.resize(numTimes, numDerivatives);
        for (int itime = 0; itime < numTimes; ++itime) {
            for (int iderivative = 0; iderivative < numDerivatives;
                    ++iderivative) {
                derivatives(itime, iderivative) =
                        tropSol.adjuncts(iderivative + numMultipliers, itime);
            }
        }
    }
    int numParameters = (int)parameter_names.size();
    // This produces an empty RowVector if numParameters is zero.
    SimTK::RowVector parameters(numParameters, tropSol.parameters.data());
    return {time, state_names, control_names, multiplier_names,
            derivative_names, parameter_names,
            states, controls, multipliers, derivatives, parameters};
}

template <typename T>
OpenSim::MucoIterate MucoTropterSolver::TropterProblemBase<T>::
convertToMucoIterate(const tropter::Iterate& tropIter) const {
    using OpenSim::MucoIterate;
    return convertIterateTropterToMuco<MucoIterate, tropter::Iterate>(tropIter);
}

template <typename T>
OpenSim::MucoSolution MucoTropterSolver::TropterProblemBase<T>::
convertToMucoSolution(const tropter::Solution& tropSol) const {
    // TODO enhance when solution contains more info than iterate.
    using OpenSim::MucoSolution;
    using tropter::Solution;
    return convertIterateTropterToMuco<MucoSolution, Solution>(tropSol);
}

template <typename T>
tropter::Iterate MucoTropterSolver::TropterProblemBase<T>::
convertToTropterIterate(const OpenSim::MucoIterate& mucoIter) const {
    tropter::Iterate tropIter;
    if (mucoIter.empty()) return tropIter;

    using Eigen::Map;
    using Eigen::RowVectorXd;
    using Eigen::MatrixXd;
    using Eigen::VectorXd;

    const auto& time = mucoIter.getTime();
    tropIter.time = Map<const RowVectorXd>(&time[0], time.size());

    tropIter.state_names = mucoIter.getStateNames();
    tropIter.control_names = mucoIter.getControlNames();
    tropIter.adjunct_names = mucoIter.getMultiplierNames();
    const auto& derivativeNames = mucoIter.getDerivativeNames();
    tropIter.adjunct_names.insert(tropIter.adjunct_names.end(),
            derivativeNames.begin(), derivativeNames.end());
    tropIter.parameter_names = mucoIter.getParameterNames();

    int numTimes = (int)time.size();
    int numStates = (int)tropIter.state_names.size();
    int numControls = (int)tropIter.control_names.size();
    int numMultipliers = (int)mucoIter.getMultiplierNames().size();
    int numDerivatives = (int)derivativeNames.size();
    int numParameters = (int)tropIter.parameter_names.size();
    const auto& states = mucoIter.getStatesTrajectory();
    const auto& controls = mucoIter.getControlsTrajectory();
    const auto& multipliers = mucoIter.getMultipliersTrajectory();
    const auto& derivatives = mucoIter.getDerivativesTrajectory();
    const auto& parameters = mucoIter.getParameters();
    // Muscollo's matrix is numTimes x numStates;
    // tropter's is numStates x numTimes.
    if (numStates) {
        tropIter.states = Map<const MatrixXd>(
                &states(0, 0), numTimes, numStates).transpose();
    } else {
        tropIter.states.resize(numStates, numTimes);
    }
    if (numControls) {
        tropIter.controls = Map<const MatrixXd>(
                &controls(0, 0), numTimes, numControls).transpose();
    } else {
        tropIter.controls.resize(numControls, numTimes);
    }

    tropIter.adjuncts.resize(numMultipliers + numDerivatives, numTimes);
    if (numMultipliers) {
        tropIter.adjuncts.topRows(numMultipliers) = Map<const MatrixXd>(
                &multipliers(0, 0), numTimes, numMultipliers).transpose();
    }
    if (numDerivatives) {
        tropIter.adjuncts.bottomRows(numDerivatives) = Map<const MatrixXd>(
                &derivatives(0, 0), numTimes, numDerivatives).transpose();
    }

    if (numParameters) {
        tropIter.parameters = Map<const VectorXd>(
                &parameters(0), numParameters);
    } else {
        tropIter.parameters.resize(numParameters);
    }
    return tropIter;
}

// Explicit template instantiations.
// ---------------------------------
template class MucoTropterSolver::TropterProblemBase<double>;
