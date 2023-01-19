//
// Copyright (C) 2004 Tanguy Fautré.
// For conditions of distribution and use,
// see copyright notice in tri_stripper.h
//
//////////////////////////////////////////////////////////////////////

#ifndef TRI_STRIPPER_HEADER_GUARD_CONNECTIVITY_GRAPH_H
#define TRI_STRIPPER_HEADER_GUARD_CONNECTIVITY_GRAPH_H

#include <vendor/TriStripper/public_types.h>

#include <vendor/TriStripper/detail/graph_array.h>
#include <vendor/TriStripper/detail/types.h>




namespace triangle_stripper
{

	namespace detail
	{

		void make_connectivity_graph(graph_array<triangle> & Triangles, const indices & Indices);

	}

}




#endif // TRI_STRIPPER_HEADER_GUARD_CONNECTIVITY_GRAPH_H
