#ifndef _cwipc_util_vectors_h_
#define _cwipc_util_vectors_h_
#include "cwipc_util/api.h"

inline cwipc_vector* add_vectors(cwipc_vector a, cwipc_vector b, cwipc_vector *result) {
    if (result) {
        result->x = a.x + b.x;
        result->y = a.y + b.y;
        result->z = a.z + b.z;
    }

    return result;
}

inline cwipc_vector* diff_vectors(cwipc_vector a, cwipc_vector b, cwipc_vector *result) {
    if (result) {
        result->x = a.x - b.x;
        result->y = a.y - b.y;
        result->z = a.z - b.z;
    }

    return result;
}

inline double len_vector(cwipc_vector v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

inline cwipc_vector* mult_vector(double factor, cwipc_vector *v) {
    if (v) {
        v->x *= factor;
        v->y *= factor;
        v->z *= factor;
    }

    return v;
}

inline cwipc_vector* norm_vector(cwipc_vector *v) {
    double len = len_vector(*v);

    if (len > 0) {
        mult_vector(1.0/len, v);
    }

    return v;
}

inline double dot_vectors(cwipc_vector a, cwipc_vector b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline cwipc_vector* cross_vectors(cwipc_vector a, cwipc_vector b, cwipc_vector *result) {
    if (result) {
        result->x = a.y*b.z - a.z*b.y;
        result->y = a.z*b.x - a.x*b.z;
        result->z = a.x*b.y - a.y*b.x;
    }

    return result;
}

#endif
