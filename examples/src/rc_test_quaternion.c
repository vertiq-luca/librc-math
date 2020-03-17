/**
 * @example    rc_test_quaternion.c
 *
 * @brief      Tests the functions in rc_quaternion.h
 *
 * @author     James Strawson
 * @date       1/29/2018
 */

#include <stdio.h>
#include <rc_math.h>


int main()
{
    double t;
    rc_vector_t q1 = RC_VECTOR_INITIALIZER;
    rc_vector_t q2 = RC_VECTOR_INITIALIZER;
    rc_vector_t q3 = RC_VECTOR_INITIALIZER;
    rc_matrix_t R  = RC_MATRIX_INITIALIZER;

    printf("\nRandom quaternions q1 and q2\n");
    rc_vector_random(&q1,4);
    rc_quaternion_normalize(&q1);
    printf("q1: ");
    rc_vector_print(q1);
    rc_vector_random(&q2,4);
    rc_quaternion_normalize(&q2);
    printf("q2: ");
    rc_vector_print(q2);

    printf("\nconvert q1 to rotation matrix R\n");
    rc_quaternion_to_rotation_matrix(q1, &R);
    rc_matrix_print(R);

    printf("\nconvert rotation matrix R back to quaternion q3\n");
    rc_rotation_to_quaternion(R, &q3);
    rc_vector_print(q3);

    printf("\nconvert q3 back to rotation matrix R\n");
    rc_quaternion_to_rotation_matrix(q3, &R);
    rc_matrix_print(R);

    printf("\ninterpolate between q1 and q2\n");
    t = 0;
    rc_quaternion_slerp(q1, q2, t, &q3);
    printf("t=%0.2f: ",t);
    rc_vector_print(q3);

    t = 0.25;
    rc_quaternion_slerp(q1, q2, t, &q3);
    printf("t=%0.2f: ",t);
    rc_vector_print(q3);

    t = .5;
    rc_quaternion_slerp(q1, q2, t, &q3);
    printf("t=%0.2f: ",t);
    rc_vector_print(q3);

    t = .75;
    rc_quaternion_slerp(q1, q2, t, &q3);
    printf("t=%0.2f: ",t);
    rc_vector_print(q3);

    t = 1.0;
    rc_quaternion_slerp(q1, q2, t, &q3);
    printf("t=%0.2f: ",t);
    rc_vector_print(q3);


    rc_vector_free(&q1);
    rc_vector_free(&q2);
    rc_vector_free(&q3);
    rc_matrix_free(&R);
    printf("\nDONE\n");
    return 0;
}
