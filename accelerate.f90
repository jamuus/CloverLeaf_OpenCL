!Crown Copyright 2012 AWE.
!
! This file is part of CloverLeaf.
!
! CloverLeaf is free software: you can redistribute it and/or modify it under 
! the terms of the GNU General Public License as published by the 
! Free Software Foundation, either version 3 of the License, or (at your option) 
! any later version.
!
! CloverLeaf is distributed in the hope that it will be useful, but 
! WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
! FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
! details.
!
! You should have received a copy of the GNU General Public License along with 
! CloverLeaf. If not, see http://www.gnu.org/licenses/.

!>  @brief Driver for the acceleration kernels
!>  @author Wayne Gaudin
!>  @details Calls user requested kernel

MODULE accelerate_module

CONTAINS

SUBROUTINE accelerate()

  USE clover_module

  IMPLICIT NONE

  INTEGER :: c

  DO c=1,number_of_chunks

    IF(chunks(c)%task.EQ.parallel%task) THEN

      CALL accelerate_kernel_ocl(chunks(c)%field%x_min,            &
                           chunks(c)%field%x_max,                  &
                           chunks(c)%field%y_min,                  &
                           chunks(c)%field%y_max,                  &
                           dt                                      )

    ENDIF

  ENDDO

END SUBROUTINE accelerate

END MODULE accelerate_module
