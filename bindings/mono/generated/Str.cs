// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Runtime.InteropServices;

#region Autogenerated code
	public class Str {

		[DllImport("libredcarpet")]
		static extern bool rc_str_case_equal(IntPtr v1, IntPtr v2);

		public static bool CaseEqual(IntPtr v1, IntPtr v2) {
			bool raw_ret = rc_str_case_equal(v1, v2);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern uint rc_str_case_hash(IntPtr key);

		public static uint CaseHash(IntPtr key) {
			uint raw_ret = rc_str_case_hash(key);
			uint ret = raw_ret;
			return ret;
		}

#endregion
	}
}