/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_THUNK64_H___
#define __INCLUDED_THUNK64_H___

/**
 * \file thunk64.h
 * \author Victor Schappert
 * \since 20140710
 * \brief Shim invoked according to the Windows 64-bit ABI that wraps a callback
 *        function
 */

 #include "thunk.h"
 
 #include "register64.h"
 
 namespace jsdi {
 namespace abi_amd64 {
 
 struct thunk64_impl;
 
 /**
  * \brief Shim invoked via Windows x64 ABI that wraps a callback function
  * \author Victor Schappert
  * \since 20140710
  */
 class thunk64 : public thunk<uint64_t>
 {
        //
        // DATA
        //

        std::unique_ptr<thunk64_impl> d_impl;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructs an x64 ABI thunk
         * \param callback_ptr Valid pointer to the callback to invoke when
         *        #func_addr() is called
         * \param num_param_registers Number of parameters that will be passed
         *        to the thunk in registers <em>must be in the range
         *        [0..NUM_PARAM_REGISTERS]
         * \param register_types Register types of the parameter registers
         */
        thunk64(const std::shared_ptr<callback_t>& callback_ptr,
                size_t num_param_registers,
                param_register_types register_types);

        //
        // ANCESTOR CLASS: thunk<uint64_t>
        //

    public:

        /**
         * \brief Returns the address of the dynamically-generated x64 ABI
         *        thunk function
         * \return Thunk function address
         */
        void * func_addr();
 };
 
 } // namespace abi_amd64
 } // namespace jsdi
 
 
#endif // __INCLUDED_THUNK64_H___
