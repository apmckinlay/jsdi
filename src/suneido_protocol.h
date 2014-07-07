/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_SUNEIDO_PROTOCOL_H___
#define __INCLUDED_SUNEIDO_PROTOCOL_H___

/**
 * \file suneido_protocol.h
 * \author Victor Schappert
 * \since 20140210
 * \brief Ability to register COM interface to handle <dfn>'suneido://'</dfn>
 *        protocol for IE.
 */
 
#include <jni.h>

namespace jsdi {

/**
 * \brief Contains functions for registering/unregistering a COM interface to
 *        handle the <dfn>'suneido://'</dfn> protocol in embedded Microsoft
 *        Internet Explorer browsers.
 * \author Victor Schappert
 * \since 20140210
 */
struct suneido_protocol
{
        /**
         * \brief Register COM interface to handle <dfn>'suneido://'</dfn>
         *        protocol.
         * \param jni_jvm Pointer to the Java virtual machine with which the
         *                handler will be associated.
         * \throws std::runtime_error If the handler can't be registered.
         * \see #unregister_handler()
         *
         * This function should be called on program startup or DLL load.
         *
         * The JVM pointer is required because the thread on which the handler
         * gets called needs to be able to get a valid <dfn>JNIEnv *</dfn>
         * pointer in order to call into Suneido.
         */
        static void register_handler(JavaVM * jni_jvm);

        /**
         * \brief Unregister COM interface that handles <dfn>'suneido://'</dfn>
         *        protocol.
         * \see #register_handler()
         *
         * If #register_handler() was called on program startup or DLL load,
         * this function should be called on program exit or DLL unload.
         */
        static void unregister_handler() noexcept;
};

} // namespace jsdi

#endif // __INCLUDED_SUNEIDO_PROTOCOL_H___
