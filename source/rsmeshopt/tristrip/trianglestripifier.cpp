/*

A general purpose stripifier, based on NvTriStrip (http://developer.nvidia.com/)

Credit for porting NvTriStrip to Python goes to the RuneBlade Foundation
library:
http://techgame.net/projects/Runeblade/browser/trunk/RBRapier/RBRapier/Tools/Geometry/Analysis/TriangleStripifier.py?rev=760

The algorithm of this stripifier is an improved version of the RuneBlade
Foundation / NVidia stripifier; it makes no assumptions about the
underlying geometry whatsoever and is intended to produce valid
output in all circumstances.

*/

/*

Copyright (c) 2007-2009, Python File Format Interface
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials provided
     with the distribution.

   * Neither the name of the Python File Format Interface
     project nor the names of its contributors may be used to endorse
     or promote products derived from this software without specific
     prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

//#define DEBUG 1 // XXX remove when done debugging

#include <algorithm> // std::copy
#include <iterator> // std:front_inserter std::back_inserter
#include <vector>

#include "trianglestripifier.hpp"

#ifdef DEBUG
#include <iostream>
#endif

TriangleStrip::TriangleStrip(int _experiment_id)
	: faces(), vertices(), reversed(false),
	  strip_id(TriangleStrip::NUM_STRIPS++),
	  experiment_id(_experiment_id) {};

bool TriangleStrip::is_face_marked(MFacePtr face)
{
	// does it belong to a final strip?
	bool result = (face->strip_id != -1);
	// it does not belong to a final strip... does it
	// belong to the current experiment?
	if ((!result) && (experiment_id != -1)) {
		result = (face->experiment_id == experiment_id);
	};
	return result;
}

void TriangleStrip::mark_face(MFacePtr face)
{
	if (experiment_id != -1) {
		face->strip_id = -1;
		face->experiment_id = experiment_id;
		face->test_strip_id = strip_id;
	} else {
		face->strip_id = strip_id;
		face->experiment_id = -1;
		face->test_strip_id = -1;
	}
}

MFacePtr TriangleStrip::get_unmarked_adjacent_face(MFacePtr face, int vi)
{
	for(std::weak_ptr<MFace> _otherface : face->get_adjacent_faces(vi)) {
		if (MFacePtr otherface = _otherface.lock()) {
			if (!is_face_marked(otherface)) {
				return otherface;
			}
		}
	}
	return MFacePtr();
}

int TriangleStrip::traverse_faces(int start_vertex, MFacePtr start_face,
                                  bool forward)
{
	int count = 0;
	int pv0 = start_vertex;
	int pv1 = start_face->get_next_vertex(pv0);
	int pv2 = start_face->get_next_vertex(pv1);
#ifdef DEBUG
#if 0
	// XXX debug
	std::cout << "starting traversal from " << pv0 << ", " << pv1 << ", " << pv2 << std::endl;
#endif
#endif
	MFacePtr next_face = get_unmarked_adjacent_face(start_face, pv0);
	while (next_face) {
		// XXX the nvidia stripifier says the following:
		// XXX
		// XXX   this tests to see if a face is "unique",
		// XXX   meaning that its
		// XXX   vertices aren't already in the list
		// XXX   so, strips which "wrap-around" are not allowed
		// XXX
		// XXX not sure why this is a problem, or, if it would
		// XXX be a problem, why nvtristrip only
		// XXX checks this only during backward traversal, so
		// XXX I simple ignore
		// XXX this (rare) problem for now :-)
		/* if not BreakTest(NextFace): break */
		// mark face, and update count
		mark_face(next_face);
		count++;
		// update vertex indices for next_face so that strip
		// continues along edge opposite pv0, add vertex to
		// strip, and add face to strip
		if (count & 1) {
			if (forward) {
				//   pv1--x            pv0-pv1
				//   / \ /   becomes   / \ /
				// pv0-pv2            x--pv2
				pv0 = pv1;
				pv1 = next_face->get_next_vertex(pv0);
				vertices.push_back(pv1);
				faces.push_back(next_face);
			} else {
				// x--pv2           pv2-pv0
				//  \ / \   becomes   \ / \
				//  pv1-pv0           pv1--x
				pv0 = pv2;
				pv2 = next_face->get_next_vertex(pv1);
				vertices.push_front(pv2);
				reversed = !reversed; // swap winding
				faces.push_front(next_face);
			}
		} else {
			if (forward) {
				//  pv0-pv1             x--pv1
				//  / \ / \   becomes  / \ / \
				// x--pv2--x          x--pv0-pv2
				pv0 = pv2;
				pv2 = next_face->get_next_vertex(pv1);
				vertices.push_back(pv2);
				faces.push_back(next_face);
			} else {
				//  pv2-pv0             pv2--x
				//  / \ / \   becomes   / \ / \
				// x--pv1--x          pv1-pv0--x
				pv0 = pv1;
				pv1 = next_face->get_next_vertex(pv0);
				vertices.push_front(pv1);
				reversed = !reversed; // swap winding
				faces.push_front(next_face);
			}
		}
#ifdef DEBUG
#if 0
		// XXX debug
		std::cout << " traversing face " << pv0 << ", " << pv1 << ", " << pv2 << std::endl;
		if (pv1 != next_face->get_next_vertex(pv0))
			throw std::runtime_error("Traversal bug.");
		if (pv2 != next_face->get_next_vertex(pv1))
			throw std::runtime_error("Traversal bug.");
		if (pv0 != next_face->get_next_vertex(pv2))
			throw std::runtime_error("Traversal bug.");
#endif
#endif
		// get next face opposite pv0
		next_face = get_unmarked_adjacent_face(next_face, pv0);
	};
	return count;
};

int TriangleStrip::build(int start_vertex, MFacePtr start_face)
{
	faces.clear();
	vertices.clear();
	reversed = false;
	// find start indices
	int v0 = start_vertex;
	int v1 = start_face->get_next_vertex(v0);
	int v2 = start_face->get_next_vertex(v1);
	// mark start face and add it to faces and strip
	mark_face(start_face);
	faces.push_back(start_face);
	vertices.push_back(v0);
	vertices.push_back(v1);
	vertices.push_back(v2);
	// build strip by traversing faces forward, then backward
	traverse_faces(v0, start_face, true);
	return traverse_faces(v2, start_face, false);
};

void TriangleStrip::commit()
{
	// remove experiment tag from strip and from its faces
	experiment_id = -1;
	for(MFacePtr face : faces) mark_face(face);
};

std::deque<int> TriangleStrip::get_strip()
{
	std::deque<int> result;
	if (reversed) {
		if (vertices.size() & 1) {
			// odd length: change winding by reversing
			// reverse is established through front_inserter
			std::copy(vertices.begin(), vertices.end(),
			          std::front_inserter(result));
		} else if (vertices.size() == 4) {
			// length 4: we can change winding without
			// appending a vertex
			result.push_back(vertices[0]);
			result.push_back(vertices[2]);
			result.push_back(vertices[1]);
			result.push_back(vertices[3]);
		} else {
			// all other cases: append duplicate vertex to
			// front
			std::copy(vertices.begin(), vertices.end(),
			          std::back_inserter(result));
			result.push_front(vertices.front());
		};
	} else {
		std::copy(vertices.begin(), vertices.end(),
		          std::back_inserter(result));
	};
	return result;
}

int TriangleStrip::NUM_STRIPS = 0;

Experiment::Experiment(int _vertex, MFacePtr _face)
	: vertex(_vertex), face(_face),
	  experiment_id(Experiment::NUM_EXPERIMENTS++) {};

void Experiment::build()
{
	// build initial strip
	TriangleStripPtr strip(new TriangleStrip(experiment_id));
	strip->build(vertex, face);
	strips.push_back(strip);
	// build strips adjacent to the initial strip, from both sides
	int num_faces = strip->faces.size();
	if (num_faces >= 4) {
		build_adjacent(strip, num_faces / 2);
		build_adjacent(strip, num_faces / 2 + 1);
	} else if (num_faces == 3) {
		// also try second edge from long side of the strip if
		// first edge fails
		if (!build_adjacent(strip, 0)) build_adjacent(strip, 2);
		build_adjacent(strip, 1);
	} else if (num_faces == 2) {
		// try to find a parallel strip from both sides
		build_adjacent(strip, 0);
		build_adjacent(strip, 1);
	} else if (num_faces == 1) {
		// try to find a parallel strip; note that there are
		// three directions to build a parallel strip to a
		// single triangle, but this is already taken care off
		// by having three experiments per triangle, one for
		// each edge
		build_adjacent(strip, 0);
	}
};

bool Experiment::build_adjacent(TriangleStripPtr strip, int face_index)
{
	//               zzzzzzzzzzzzzz
	// otherface:      /         \
	//            othervertex---nextvertex--yyyyyyyy
	// strip:           \        /      \   /    \
	//                oppositevertex----xxxxxxx----......
	//
	//            .........---oppositevertex-yyyyyyyy
	// strip:           \        /      \     /    \
	//                othervertex----nextvertex-----......
	// otherface:                \       /
	//                         zzzzzzzzzzzzzz
	//
	// and so on...

	int oppositevertex = strip->vertices[face_index + 1];
	MFacePtr face = strip->faces[face_index];
	if (MFacePtr otherface = strip->get_unmarked_adjacent_face(face, oppositevertex)) {
		bool winding = strip->reversed; // winding of first face
		if (face_index & 1) winding = !winding;
		// create and build new strip
		TriangleStripPtr otherstrip(new TriangleStrip(experiment_id));
		if (winding) {
			int othervertex = strip->vertices[face_index];
			face_index = otherstrip->build(othervertex, otherface);
		} else {
			int nextvertex = strip->vertices[face_index + 2];
			face_index = otherstrip->build(nextvertex, otherface);
		};
		strips.push_back(otherstrip);
		// build adjacent strip to the strip we just found
		if (face_index > otherstrip->faces.size() / 2) {
			build_adjacent(otherstrip, face_index - 1);
		} else if (face_index < otherstrip->faces.size() - 1) {
			build_adjacent(otherstrip, face_index + 1);
		}
		// we built an adjacent strip
		return true;
	};
	return false;
}

int Experiment::NUM_EXPERIMENTS = 0;

ExperimentSelector::ExperimentSelector(int _num_samples, int _min_strip_length)
	: num_samples(_num_samples), min_strip_length(_min_strip_length),
	  strip_len_heuristic(1.0), best_score(0.0), best_sample() {};

void ExperimentSelector::update_score(ExperimentPtr experiment)
{
	// score is average number of faces per strip
	// XXX experiment with other scoring rules?
	int stripsize = 0;
	for(TriangleStripPtr strip : experiment->strips) {
		stripsize += strip->faces.size();
	};
	float score = ((strip_len_heuristic * stripsize)
	               / experiment->strips.size());
	if (score > best_score) {
		best_score = score;
		best_sample = experiment;
	};
}

void ExperimentSelector::clear()
{
	best_score = 0.0;
	best_sample.reset();
}

TriangleStripifier::TriangleStripifier(MeshPtr _mesh)
	: selector(10, 0), mesh(_mesh), start_face_iter(_mesh->faces.end()) {};

bool TriangleStripifier::find_good_reset_point()
{
	// special case: no faces!
	if (mesh->faces.size() == 0)
		return false;

	int start_step = mesh->faces.size() / selector.num_samples;
	if ((mesh->faces.end() - start_face_iter) > start_step) {
		start_face_iter += start_step;
	} else {
		start_face_iter = mesh->faces.begin() + (start_step - (mesh->faces.end() - start_face_iter));
	};
	const MFacePtr* face = mesh->faces.data() + (start_face_iter - mesh->faces.begin());
	const MFacePtr* sentinel = &*start_face_iter;
	do {
		if ((*face)->strip_id == -1) {
			// face not used in any strip, so start there for next strip
			start_face_iter = mesh->faces.begin() + (face - mesh->faces.data());
			return true;
		};
		face++;
		if (face == mesh->faces.data() + mesh->faces.size())
			face = mesh->faces.data();
	} while (face != sentinel);
	// we have exhausted all the faces
	return false;
};

std::list<TriangleStripPtr> TriangleStripifier::find_all_strips()
{
	std::list<TriangleStripPtr> all_strips;

	while (true) {
		// note: one experiment is a collection of adjacent strips
		std::list<ExperimentPtr> experiments;
		std::set<Face> visited_reset_points;
		for (int n_sample = 0; n_sample < selector.num_samples; n_sample++) {
			// Get a good start face for an experiment
			if (!find_good_reset_point()) {
				// done!
				break;
			};
			MFacePtr exp_face = *start_face_iter;
			// XXX this is an ugly pointer hack...
			// XXX do we need to check this?
			Face *exp_face_index = dynamic_cast<Face *>(&(**start_face_iter));
			if (visited_reset_points.find(*exp_face_index) != visited_reset_points.end()) {
				// We've seen this face already... try again
				continue;
			}
			visited_reset_points.insert(*exp_face_index);
			// Create an exploration from ExpFace in each of the three directions
			int vertices[] = {exp_face->v0, exp_face->v1, exp_face->v2};
			for (int exp_vertex : vertices) {
				// Create the seed strip for the experiment
				ExperimentPtr exp(new Experiment(exp_vertex, exp_face));
				// Add the seeded experiment list to the experiment collection
				experiments.push_back(exp);
			}
		}
		if (experiments.empty()) {
			// no more experiments to run: done!!
			return all_strips;
		};
		// note: iterate via reference, so we can clear the experiment
		for(ExperimentPtr & exp : experiments) {
			exp->build();
			selector.update_score(exp);
			exp.reset(); // no reason to keep
		};
		experiments.clear(); // no reason to keep
		// Get the best experiment according to the selector
		ExperimentPtr best_experiment = selector.best_sample;
		selector.clear();
		// And commit it to the resultset
		for (TriangleStripPtr strip : best_experiment->strips) {
			strip->commit();
			all_strips.push_back(strip);
		}
		best_experiment.reset();
	}
}
