using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using UnityEngine;

namespace cwipc
{
    using Timestamp = System.Int64;

    public struct FrameInfo
    {
        // presentation timestamp, in milliseconds units.
        public Timestamp timestamp;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 256)]
        public byte[] dsi;
        public int dsi_size;
    }

    public class BaseMemoryChunkReferences
    {
        static List<Type> types = new List<Type>();
        public static void AddReference(Type _type)
        {
            lock (types)
            {
                types.Add(_type);
            }
        }
        public static void DeleteReference(Type _type)
        {
            lock (types)
            {
                types.Remove(_type);
            }
        }

        public static void ShowTotalRefCount()
        {
            lock (types)
            {
                if (types.Count == 0) return;
                Debug.Log($"BaseMemoryChunkReferences: {types.Count} TotalRefCount pending:");
                for (int i = 0; i < types.Count; ++i)
                    Debug.Log($"BaseMemoryChunkReferences: [{i}] --> {types[i]}");
            }
        }
    }

    
    public class BaseMemoryChunk
    {

        protected IntPtr _pointer;
        int refCount;
        public FrameInfo info;
        public int length { get; protected set; }

        protected BaseMemoryChunk(IntPtr _pointer)
        {
            if (_pointer == IntPtr.Zero) throw new Exception("BaseMemoryChunk: constructor called with null pointer");
            this._pointer = _pointer;
            refCount = 1;
            BaseMemoryChunkReferences.AddReference(GetType());
        }

        protected BaseMemoryChunk()
        {
            // _pointer will be set later, in the subclass constructor. Not a pattern I'm happy with but difficult to
            refCount = 1;
            BaseMemoryChunkReferences.AddReference(GetType());
        }


        public BaseMemoryChunk AddRef()
        {
            lock (this)
            {
                refCount++;
                return this;
            }
        }
        public IntPtr pointer
        {
            get
            {
                lock (this)
                {
                    if (refCount <= 0)
                    {
                        throw new Exception($"BaseMemoryChunk.pointer: refCount={refCount}");
                    }
                    return _pointer;
                }
            }
        }

        public int free()
        {
            lock (this)
            {
                if (--refCount < 1)
                {
                    if (refCount < 0)
                    {
                        throw new Exception($"BaseMemoryChunk.free: refCount={refCount}");
                    }
                    if (_pointer != IntPtr.Zero)
                    {
                        refCount = 1;   // Temporarily increase refcount so onfree() can use pointer.
                        onfree();
                        refCount = 0;
                        _pointer = IntPtr.Zero;
                        BaseMemoryChunkReferences.DeleteReference(GetType());
                    }
                }
                return refCount;
            }
        }

        protected virtual void onfree()
        {
        }
    }
}