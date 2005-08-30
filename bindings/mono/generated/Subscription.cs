// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Runtime.InteropServices;

#region Autogenerated code
	public class Subscription {

		[DllImport("libredcarpet")]
		static extern bool rc_subscription_get_status(IntPtr channel);

		public static bool GetStatus(RC.Channel channel) {
			bool raw_ret = rc_subscription_get_status(channel == null ? IntPtr.Zero : channel.Handle);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_subscription_set_file(IntPtr file);

		public static string File { 
			set {
				IntPtr file_as_native = GLib.Marshaller.StringToPtrGStrdup (value);
				rc_subscription_set_file(file_as_native);
				GLib.Marshaller.Free (file_as_native);
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_subscription_set_status(IntPtr channel, bool channel_is_subscribed);

		public static void SetStatus(RC.Channel channel, bool channel_is_subscribed) {
			rc_subscription_set_status(channel == null ? IntPtr.Zero : channel.Handle, channel_is_subscribed);
		}

#endregion
	}
}
