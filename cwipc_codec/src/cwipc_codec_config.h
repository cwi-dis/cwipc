
//
// Defining this leads to a 50% performance increase, but breaks P coding.
// It also seems to break a lot more.
//
#undef CWIPC_CODEC_WITH_SINGLE_BUF


#undef CWIPC_CODEC_DELETE_BUFFER_IN_ENCODER
#define CWIPC_CODEC_DELETE_TREE_IN_ENCODER

//#ifdef WIN32
//#define CWIPC_CODEC_DELETE_TREE_IN_ENCODER
//#endif
