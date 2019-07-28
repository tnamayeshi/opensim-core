#ifndef MOCO_MODELFACTORY_H
#define MOCO_MODELFACTORY_H
/* -------------------------------------------------------------------------- *
 * OpenSim Moco: ModelFactory.h                                               *
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

#include "../osimMocoDLL.h"

#include <OpenSim/Simulation/Model/Model.h>

namespace OpenSim {

/// This class provides utilities for creating OpenSim models.
class OSIMMOCO_API ModelFactory {
public:
    /// @name Create a model
    /// @{

    /// Create a pendulum with the provided number of links.
    /// For each link, there is a body `/bodyset/b#` (where `#` is the link
    /// index starting at 0), a PinJoint `/jointset/j#` with coordinate
    /// `/jointset/j#/q#`, a CoordinateActuator `/tau#`, a Marker
    /// `/markerset/marker#` at the origin of the link's body, and a
    /// PhysicalOffsetFrame \c /b\#center at the center of the link.
    static Model createNLinkPendulum(int numLinks);
    /// This is a convenience for `createNLinkPendulum(1)`.static Model createPendulum() { return createNLinkPendulum(1); }
    /// This is a convenience for `createNLinkPendulum(2)`.
    static Model createDoublePendulum() { return createNLinkPendulum(2); }
    /// This model contains:
    /// - 2 bodies: a massless body "intermed", and "body" with mass 1.
    /// - 2 slider joints: "tx" and "ty" (coordinates "tx" and "ty").
    /// - 2 coordinate actuators: "force_x" and "force_y".
    static Model createPlanarPointMass();
    /// This model contains differential equations for the Brachistochrone
    /// optimal control problem. The model contains a single component of type
    /// Brachistochrone (defined in the function) with states x, y, v, and
    /// control w. The constant g is std::abs(Model().get_gravity()[1]).
    ///     xdot = v * cos(w)
    ///     ydot = v * sin(w)
    ///     vdot = g * sin(w)
    /// These equations are from Betts, 2010 "Practical Methods for Optimal
    /// Control and Estimation using Nonlinear Programming" Example 4.10 (page
    /// 215).
    static Model createBrachistochrone();

    /// @}

    /// @name Modify a Model
    /// @{

    /// Replace muscles in a model with a PathActuator of the same GeometryPath,
    /// optimal force, and min/max control defaults.
    /// @note This only replaces muscles within the model's ForceSet.
    static void replaceMusclesWithPathActuators(Model& model);

    /// Remove muscles from the model.
    /// @note This only removes muscles within the model's ForceSet.
    static void removeMuscles(Model& model);

    /// Replace a joint in the model with a WeldJoint.
    /// @note This assumes the joint is in the JointSet and that the joint's
    ///       connectees are PhysicalOffsetFrames.
    static void replaceJointWithWeldJoint(
            Model& model, const std::string& jointName);

    /// Add CoordinateActuator%s for each unconstrained coordinate (e.g.,
    /// `! Coordinate::isConstrained()`) in the model, using the provided optimal
    /// force. Increasing the optimal force decreases the required control
    /// signal to generate a given actuation level. The actuators are added to
    /// the model's ForceSet and are named "reserve_<coordinate-path>" with
    /// forward slashes converted to underscores. The third optional argument
    /// specifies whether or not to skip coordinates that already have
    /// CoordinateActuator%s associated with them (default: true).
    static void createReserveActuators(Model& model, double optimalForce,
            bool skipCoordinatesWithExistingActuators = true);

    /// @}
};

} // namespace OpenSim

#endif // MOCO_MODELFACTORY_H
