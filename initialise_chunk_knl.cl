/*Crown Copyright 2012 AWE.
*
* This file is part of CloverLeaf.
*
* CloverLeaf is free software: you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the
* Free Software Foundation, either version 3 of the License, or (at your option)
* any later version.
*
* CloverLeaf is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along with
* CloverLeaf. If not, see http://www.gnu.org/licenses/. */

/**
 *  @brief OCL device-side initialise chunk kernels
 *  @author Andrew Mallinson, David Beckingsale, Wayne Gaudin
 *  @details Invokes the user specified chunk initialisation kernels.
 */

#include "ocl_knls.h"

__kernel void initialise_chunk_cell_x_ocl_kernel(
    const double dx,
    __global const double * restrict vertexx,
    __global double * restrict cellx,
    __global double * restrict celldx)
{

    int j = get_global_id(0)-2;

    if (j<=XMAX+2) {

        cellx[ARRAY1D(j, XMIN-2)] = 0.5 * ( vertexx[ARRAY1D(j, XMIN-2)] + vertexx[ARRAY1D(j+1, XMIN-2)] );
        
        celldx[ARRAY1D(j, XMIN-2)] = dx;

    }
}


__kernel void initialise_chunk_cell_y_ocl_kernel(
    const double dy,
    __global const double * restrict vertexy,
    __global double * restrict celly,
    __global double * restrict celldy)
{

    int k = get_global_id(0)-2;
    
    if (k<=YMAX+2) {

        celly[ARRAY1D(k, YMIN-2)] = 0.5 * ( vertexy[ARRAY1D(k, YMIN-2)] + vertexy[ARRAY1D(k+1, YMIN-2)] );
        
        celldy[ARRAY1D(k, YMIN-2)] = dy;
    }
}


__kernel void initialise_chunk_vertex_x_ocl_kernel(
    const double xmin,
    const double dx,
    __global double * restrict vertexx,
    __global double * restrict vertexdx)
{

    int j = get_global_id(0)-2;

    if (j<=XMAX+3) {

        vertexx[ARRAY1D(j,XMIN-2)] = xmin + dx * (j-XMIN);
        vertexdx[ARRAY1D(j,XMIN-2)] = dx;

    }
}


__kernel void initialise_chunk_vertex_y_ocl_kernel(
    const double ymin,
    const double dy,
    __global double * restrict vertexy,
    __global double * restrict vertexdy)
{  

    int k = get_global_id(0)-2;

    if (k<=YMAX+3) {

        vertexy[ARRAY1D(k, YMIN-2)] = ymin + dy * (k-YMIN);

        vertexdy[ARRAY1D(k, YMIN-2)] = dy;

    }

}

__kernel void initialise_chunk_volume_area_ocl_kernel(
    const double dx,
    const double dy,
    __global double * restrict volume,
    __global const double * restrict celldx,
    __global const double * restrict celldy,
    __global double * restrict xarea,
    __global double * restrict yarea)
{   
    int k = get_global_id(1);
    int j = get_global_id(0);

    if ( (j<=XMAXPLUSTHREE) && (k<=YMAXPLUSTHREE) ) {
    
        volume[ARRAYXY(j,k, XMAXPLUSFOUR)] = dx * dy;

        xarea[ARRAYXY(j,k, XMAXPLUSFIVE)] = celldy[k];

        yarea[ARRAYXY(j,k, XMAXPLUSFOUR)] = celldx[j];
    }
}


