#include "ocl_common.hpp"
#include <numeric>

extern "C" void ocl_pack_buffers_
(int fields[NUM_FIELDS], int offsets[NUM_FIELDS], int * depth,
 int * face, double * buffer)
{
    if (std::accumulate(fields, fields + (NUM_FIELDS-1), 0) > 0)
    {
        // only call if there's actually something to pack
        chunk.packUnpackAllBuffers(fields, offsets, *depth, *face, 1, buffer);
    }
}

extern "C" void ocl_unpack_buffers_
(int fields[NUM_FIELDS], int offsets[NUM_FIELDS], int * depth,
 int * face, double * buffer)
{
    if (std::accumulate(fields, fields + (NUM_FIELDS-1), 0) > 0)
    {
        // only call if there's actually something to unpack
        chunk.packUnpackAllBuffers(fields, offsets, *depth, *face, 0, buffer);
    }
}

void CloverChunk::packUnpackAllBuffers
(int fields[NUM_FIELDS], int offsets[NUM_FIELDS],
 const int depth, const int face, const int pack,
 double * buffer)
{
    // which subbuffer to use - incrmement by 1 for each buffer packed
    int current_subbuf = 0;

    if (!pack)
    {
        cl::Buffer * side_buffer = NULL;
        int side_size = 0;

        switch (face)
        {
        case CHUNK_LEFT:
            side_buffer = &left_buffer;
            side_size = x_max;
            break;
        case CHUNK_RIGHT:
            side_buffer = &right_buffer;
            side_size = x_max;
            break;
        case CHUNK_BOTTOM:
            side_buffer = &bottom_buffer;
            side_size = y_max;
            break;
        case CHUNK_TOP:
            side_buffer = &top_buffer;
            side_size = y_max;
            break;
        default:
            DIE("Invalid face identifier %d passed to mpi buffer packing\n", face);
        }

        queue.enqueueWriteBuffer(*side_buffer, CL_TRUE, 0,
            NUM_BUFFERED_FIELDS*2*sizeof(double)*(side_size + 5),
            buffer);
    }

    for (int ii = 0; ii < NUM_FIELDS; ii++)
    {
        int which_field = ii+1;

        if (fields[ii])
        {
            if (offsets[ii] < 0)
            {
                DIE("Tried to pack/unpack field %d but invalid offset %d given\n",
                    ii, offsets[ii]);
            }

            int x_inc, y_inc;
            x_inc = y_inc = 0;

            // set x/y/z inc for array
            switch (which_field)
            {
            case FIELD_xvel0:
            case FIELD_yvel0:
            case FIELD_xvel1:
            case FIELD_yvel1:
                x_inc = y_inc = 1;
                break;
            case FIELD_mass_flux_x:
            case FIELD_vol_flux_x:
                x_inc = 1;
                break;
            case FIELD_mass_flux_y:
            case FIELD_vol_flux_y:
                y_inc = 1;
                break;
            case FIELD_density0:
            case FIELD_density1:
            case FIELD_energy0:
            case FIELD_energy1:
            case FIELD_pressure:
            case FIELD_viscosity:
            case FIELD_soundspeed:
                break;
            default:
                DIE("Invalid field number %d in choosing _inc values\n", which_field);
            }

            #define CASE_BUF(which_array) \
            case FIELD_##which_array: \
            { \
                device_array = &which_array; \
            }

            cl::Buffer * device_array;

            switch (which_field)
            {
            CASE_BUF(density0); break;
            CASE_BUF(density1); break;
            CASE_BUF(energy0); break;
            CASE_BUF(energy1); break;
            CASE_BUF(pressure); break;
            CASE_BUF(viscosity); break;
            CASE_BUF(soundspeed); break;
            CASE_BUF(xvel0); break;
            CASE_BUF(xvel1); break;
            CASE_BUF(yvel0); break;
            CASE_BUF(yvel1); break;
            CASE_BUF(vol_flux_x); break;
            CASE_BUF(vol_flux_y); break;
            CASE_BUF(mass_flux_x); break;
            CASE_BUF(mass_flux_y); break;
            default:
                device_array = NULL;
                DIE("Invalid face %d passed to left/right pack buffer\n", which_field);
            }

            int dest;
            cl::Kernel pack_kernel;

            // set the destination and which kernel to call
            if (pack)
            {
                switch (face)
                {
                case CHUNK_LEFT:
                    pack_kernel = pack_left_buffer_device;
                    dest = (x_min+1) + x_inc - 1 + 1; break;
                case CHUNK_RIGHT:
                    pack_kernel = pack_right_buffer_device;
                    dest = (x_max+1) + 1 - depth; break;
                case CHUNK_BOTTOM:
                    pack_kernel = pack_bottom_buffer_device;
                    dest = (y_min+1) + y_inc - 1 + 1; break;
                case CHUNK_TOP:
                    pack_kernel = pack_top_buffer_device;
                    dest = (y_max+1) + 1 - depth; break;
                default:
                    DIE("Invalid face identifier %d passed to pack\n", face);
                }
            }
            else
            {
                switch (face)
                {
                case CHUNK_LEFT:
                    pack_kernel = unpack_left_buffer_device;
                    dest = (x_min+1) - depth; break;
                case CHUNK_RIGHT:
                    pack_kernel = unpack_right_buffer_device;
                    dest = (x_max+1) + x_inc + 1; break;
                case CHUNK_BOTTOM:
                    pack_kernel = unpack_bottom_buffer_device;
                    dest = (y_min+1) - depth; break;
                case CHUNK_TOP:
                    pack_kernel = unpack_top_buffer_device;
                    dest = (y_max+1) + y_inc + 1; break;
                default:
                    DIE("Invalid face identifier %d passed to unpack\n", face);
                }
            }

            // choose the right subbuffer
            // this might cause slowdown with clretainmemoryobject?
            cl::Buffer packing_subbuf;

            switch (face)
            {
            case CHUNK_LEFT:
                packing_subbuf = left_subbuffers[depth-1].at(current_subbuf);
                break;
            case CHUNK_RIGHT:
                packing_subbuf = right_subbuffers[depth-1].at(current_subbuf);
                break;
            case CHUNK_BOTTOM:
                packing_subbuf = bottom_subbuffers[depth-1].at(current_subbuf);
                break;
            case CHUNK_TOP:
                packing_subbuf = top_subbuffers[depth-1].at(current_subbuf);
                break;
            default:
                DIE("Invalid face identifier %d passed to subbuf choice\n", face);
            }

            pack_kernel.setArg(0, x_inc);
            pack_kernel.setArg(1, y_inc);
            pack_kernel.setArg(2, *device_array);
            pack_kernel.setArg(3, packing_subbuf);
            pack_kernel.setArg(4, depth);

            cl::size_t<3> b_origin;
            cl::size_t<3> h_origin;
            cl::size_t<3> region;

            switch (face)
            {
            // depth*y_max+... region - 1 or 2 columns
            case CHUNK_LEFT:
            case CHUNK_RIGHT:
                b_origin[0] = dest;
                b_origin[1] = (y_min+1) - depth;
                b_origin[2] = 0;
                region[0] = depth;
                region[1] = y_max + y_inc + (2*depth);
                region[2] = 1;
                break;

            // depth*x_max+... region - 1 or 2 rows
            case CHUNK_BOTTOM:
            case CHUNK_TOP:
                b_origin[0] = (x_min+1) - depth;
                b_origin[1] = dest;
                b_origin[2] = 0;
                region[0] = x_max + x_inc + (2*depth);
                region[1] = depth;
                region[2] = 1;
                break;
            default:
                DIE("Invalid face identifier %d passed to mpi buffer packing\n", face);
            }

            enqueueKernel(pack_kernel, __LINE__, __FILE__,
                          cl::NDRange(b_origin[0], b_origin[1], b_origin[2]),
                          cl::NDRange(region[0], region[1], region[2]),
                          cl::NullRange);

            // next subbuffer
            current_subbuf += 1;
        }
    }

    if (pack)
    {
        // make sure kernels are finished
        queue.finish();

        cl::Buffer * side_buffer = NULL;
        int side_size = 0;

        switch (face)
        {
        case CHUNK_LEFT:
            side_buffer = &left_buffer;
            side_size = x_max;
            break;
        case CHUNK_RIGHT:
            side_buffer = &right_buffer;
            side_size = x_max;
            break;
        case CHUNK_BOTTOM:
            side_buffer = &bottom_buffer;
            side_size = y_max;
            break;
        case CHUNK_TOP:
            side_buffer = &top_buffer;
            side_size = y_max;
            break;
        default:
            DIE("Invalid face identifier %d passed to mpi buffer packing\n", face);
        }

        queue.enqueueReadBuffer(*side_buffer, CL_TRUE, 0,
            NUM_BUFFERED_FIELDS*2*sizeof(double)*(side_size + 5),
            buffer);
    }
}

