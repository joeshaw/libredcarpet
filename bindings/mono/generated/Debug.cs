// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Runtime.InteropServices;

#region Autogenerated code
	public class Debug {

		[DllImport("libredcarpet")]
		static extern void rc_debug_remove_handler(uint id);

		public static void RemoveHandler(uint id) {
			rc_debug_remove_handler(id);
		}

		[DllImport("libredcarpet")]
		static extern uint rc_debug_add_handler(RCSharp.DebugFnNative fn, int level, IntPtr user_data);

		public static uint AddHandler(RC.DebugFn fn, RC.DebugLevel level) {
			RCSharp.DebugFnWrapper fn_wrapper = null;
			fn_wrapper = new RCSharp.DebugFnWrapper (fn, null);
			uint raw_ret = rc_debug_add_handler(fn_wrapper.NativeDelegate, (int) level, IntPtr.Zero);
			uint ret = raw_ret;
			return ret;
		}

#endregion
	}
}