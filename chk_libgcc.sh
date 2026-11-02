G=$(arm-none-eabi-gcc -print-libgcc-file-name)
echo "libgcc: $G"
arm-none-eabi-nm -g "$G" 2>/dev/null | grep -E " T (_Unwind_RaiseException|_Unwind_Resume|_Unwind_VRS_Get|__gnu_unwind_frame|__aeabi_unwind_cpp_pr0|__aeabi_uidiv|__aeabi_ldivmod|__aeabi_uldivmod|__aeabi_d2f|__aeabi_dcmpeq|__aeabi_fcmpeq|__aeabi_fcmpun)$" | sort -u