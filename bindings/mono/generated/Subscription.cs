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
			bool raw_ret = rc_subscription_get_status(channel.Handle);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_subscription_set_status(IntPtr channel, bool channel_is_subscribed);

		public static void SetStatus(RC.Channel channel, bool channel_is_subscribed) {
			rc_subscription_set_status(channel.Handle, channel_is_subscribed);
		}

#endregion
	}
}