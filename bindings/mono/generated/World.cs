// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	public  class World : GLib.Object {

		~World()
		{
			Dispose();
		}

		[Obsolete]
		protected World(GLib.GType gtype) : base(gtype) {}
		public World(IntPtr raw) : base(raw) {}

		protected World() : base(IntPtr.Zero)
		{
			CreateNativeObject (new string [0], new GLib.Value [0]);
		}

		delegate void ChangedChannelsDelegate (IntPtr arg1);

		static ChangedChannelsDelegate ChangedChannelsCallback;

		static void changedchannels_cb (IntPtr arg1)
		{
			World obj = GLib.Object.GetObject (arg1, false) as World;
			obj.OnChangedChannels ();
		}

		private static void OverrideChangedChannels (GLib.GType gtype)
		{
			if (ChangedChannelsCallback == null)
				ChangedChannelsCallback = new ChangedChannelsDelegate (changedchannels_cb);
			OverrideVirtualMethod (gtype, "changed_channels", ChangedChannelsCallback);
		}

		[GLib.DefaultSignalHandler(Type=typeof(RC.World), ConnectionMethod="OverrideChangedChannels")]
		protected virtual void OnChangedChannels ()
		{
			GLib.Value ret = GLib.Value.Empty;
			GLib.ValueArray inst_and_params = new GLib.ValueArray (1);
			GLib.Value[] vals = new GLib.Value [1];
			vals [0] = new GLib.Value (this);
			inst_and_params.Append (vals [0]);
			g_signal_chain_from_overridden (inst_and_params.ArrayPtr, ref ret);
		}

		[GLib.Signal("changed_channels")]
		public event System.EventHandler ChangedChannels {
			add {
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					if (BeforeHandlers["changed_channels"] == null)
						BeforeSignals["changed_channels"] = new RCSharp.voidObjectSignal(this, "changed_channels", value, typeof (System.EventArgs), 0);
					else
						((GLib.SignalCallback) BeforeSignals ["changed_channels"]).AddDelegate (value);
					BeforeHandlers.AddHandler("changed_channels", value);
				} else {
					if (AfterHandlers["changed_channels"] == null)
						AfterSignals["changed_channels"] = new RCSharp.voidObjectSignal(this, "changed_channels", value, typeof (System.EventArgs), 1);
					else
						((GLib.SignalCallback) AfterSignals ["changed_channels"]).AddDelegate (value);
					AfterHandlers.AddHandler("changed_channels", value);
				}
			}
			remove {
				System.ComponentModel.EventHandlerList event_list = AfterHandlers;
				Hashtable signals = AfterSignals;
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					event_list = BeforeHandlers;
					signals = BeforeSignals;
				}
				GLib.SignalCallback cb = signals ["changed_channels"] as GLib.SignalCallback;
				event_list.RemoveHandler("changed_channels", value);
				if (cb == null)
					return;

				cb.RemoveDelegate (value);

				if (event_list["changed_channels"] == null) {
					signals.Remove("changed_channels");
					cb.Dispose ();
				}
			}
		}

		delegate void ChangedSubscriptionsDelegate (IntPtr arg1);

		static ChangedSubscriptionsDelegate ChangedSubscriptionsCallback;

		static void changedsubscriptions_cb (IntPtr arg1)
		{
			World obj = GLib.Object.GetObject (arg1, false) as World;
			obj.OnChangedSubscriptions ();
		}

		private static void OverrideChangedSubscriptions (GLib.GType gtype)
		{
			if (ChangedSubscriptionsCallback == null)
				ChangedSubscriptionsCallback = new ChangedSubscriptionsDelegate (changedsubscriptions_cb);
			OverrideVirtualMethod (gtype, "changed_subscriptions", ChangedSubscriptionsCallback);
		}

		[GLib.DefaultSignalHandler(Type=typeof(RC.World), ConnectionMethod="OverrideChangedSubscriptions")]
		protected virtual void OnChangedSubscriptions ()
		{
			GLib.Value ret = GLib.Value.Empty;
			GLib.ValueArray inst_and_params = new GLib.ValueArray (1);
			GLib.Value[] vals = new GLib.Value [1];
			vals [0] = new GLib.Value (this);
			inst_and_params.Append (vals [0]);
			g_signal_chain_from_overridden (inst_and_params.ArrayPtr, ref ret);
		}

		[GLib.Signal("changed_subscriptions")]
		public event System.EventHandler ChangedSubscriptions {
			add {
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					if (BeforeHandlers["changed_subscriptions"] == null)
						BeforeSignals["changed_subscriptions"] = new RCSharp.voidObjectSignal(this, "changed_subscriptions", value, typeof (System.EventArgs), 0);
					else
						((GLib.SignalCallback) BeforeSignals ["changed_subscriptions"]).AddDelegate (value);
					BeforeHandlers.AddHandler("changed_subscriptions", value);
				} else {
					if (AfterHandlers["changed_subscriptions"] == null)
						AfterSignals["changed_subscriptions"] = new RCSharp.voidObjectSignal(this, "changed_subscriptions", value, typeof (System.EventArgs), 1);
					else
						((GLib.SignalCallback) AfterSignals ["changed_subscriptions"]).AddDelegate (value);
					AfterHandlers.AddHandler("changed_subscriptions", value);
				}
			}
			remove {
				System.ComponentModel.EventHandlerList event_list = AfterHandlers;
				Hashtable signals = AfterSignals;
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					event_list = BeforeHandlers;
					signals = BeforeSignals;
				}
				GLib.SignalCallback cb = signals ["changed_subscriptions"] as GLib.SignalCallback;
				event_list.RemoveHandler("changed_subscriptions", value);
				if (cb == null)
					return;

				cb.RemoveDelegate (value);

				if (event_list["changed_subscriptions"] == null) {
					signals.Remove("changed_subscriptions");
					cb.Dispose ();
				}
			}
		}

		delegate void ChangedLocksDelegate (IntPtr arg1);

		static ChangedLocksDelegate ChangedLocksCallback;

		static void changedlocks_cb (IntPtr arg1)
		{
			World obj = GLib.Object.GetObject (arg1, false) as World;
			obj.OnChangedLocks ();
		}

		private static void OverrideChangedLocks (GLib.GType gtype)
		{
			if (ChangedLocksCallback == null)
				ChangedLocksCallback = new ChangedLocksDelegate (changedlocks_cb);
			OverrideVirtualMethod (gtype, "changed_locks", ChangedLocksCallback);
		}

		[GLib.DefaultSignalHandler(Type=typeof(RC.World), ConnectionMethod="OverrideChangedLocks")]
		protected virtual void OnChangedLocks ()
		{
			GLib.Value ret = GLib.Value.Empty;
			GLib.ValueArray inst_and_params = new GLib.ValueArray (1);
			GLib.Value[] vals = new GLib.Value [1];
			vals [0] = new GLib.Value (this);
			inst_and_params.Append (vals [0]);
			g_signal_chain_from_overridden (inst_and_params.ArrayPtr, ref ret);
		}

		[GLib.Signal("changed_locks")]
		public event System.EventHandler ChangedLocks {
			add {
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					if (BeforeHandlers["changed_locks"] == null)
						BeforeSignals["changed_locks"] = new RCSharp.voidObjectSignal(this, "changed_locks", value, typeof (System.EventArgs), 0);
					else
						((GLib.SignalCallback) BeforeSignals ["changed_locks"]).AddDelegate (value);
					BeforeHandlers.AddHandler("changed_locks", value);
				} else {
					if (AfterHandlers["changed_locks"] == null)
						AfterSignals["changed_locks"] = new RCSharp.voidObjectSignal(this, "changed_locks", value, typeof (System.EventArgs), 1);
					else
						((GLib.SignalCallback) AfterSignals ["changed_locks"]).AddDelegate (value);
					AfterHandlers.AddHandler("changed_locks", value);
				}
			}
			remove {
				System.ComponentModel.EventHandlerList event_list = AfterHandlers;
				Hashtable signals = AfterSignals;
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					event_list = BeforeHandlers;
					signals = BeforeSignals;
				}
				GLib.SignalCallback cb = signals ["changed_locks"] as GLib.SignalCallback;
				event_list.RemoveHandler("changed_locks", value);
				if (cb == null)
					return;

				cb.RemoveDelegate (value);

				if (event_list["changed_locks"] == null) {
					signals.Remove("changed_locks");
					cb.Dispose ();
				}
			}
		}

		delegate void ChangedPackagesDelegate (IntPtr arg1);

		static ChangedPackagesDelegate ChangedPackagesCallback;

		static void changedpackages_cb (IntPtr arg1)
		{
			World obj = GLib.Object.GetObject (arg1, false) as World;
			obj.OnChangedPackages ();
		}

		private static void OverrideChangedPackages (GLib.GType gtype)
		{
			if (ChangedPackagesCallback == null)
				ChangedPackagesCallback = new ChangedPackagesDelegate (changedpackages_cb);
			OverrideVirtualMethod (gtype, "changed_packages", ChangedPackagesCallback);
		}

		[GLib.DefaultSignalHandler(Type=typeof(RC.World), ConnectionMethod="OverrideChangedPackages")]
		protected virtual void OnChangedPackages ()
		{
			GLib.Value ret = GLib.Value.Empty;
			GLib.ValueArray inst_and_params = new GLib.ValueArray (1);
			GLib.Value[] vals = new GLib.Value [1];
			vals [0] = new GLib.Value (this);
			inst_and_params.Append (vals [0]);
			g_signal_chain_from_overridden (inst_and_params.ArrayPtr, ref ret);
		}

		[GLib.Signal("changed_packages")]
		public event System.EventHandler ChangedPackages {
			add {
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					if (BeforeHandlers["changed_packages"] == null)
						BeforeSignals["changed_packages"] = new RCSharp.voidObjectSignal(this, "changed_packages", value, typeof (System.EventArgs), 0);
					else
						((GLib.SignalCallback) BeforeSignals ["changed_packages"]).AddDelegate (value);
					BeforeHandlers.AddHandler("changed_packages", value);
				} else {
					if (AfterHandlers["changed_packages"] == null)
						AfterSignals["changed_packages"] = new RCSharp.voidObjectSignal(this, "changed_packages", value, typeof (System.EventArgs), 1);
					else
						((GLib.SignalCallback) AfterSignals ["changed_packages"]).AddDelegate (value);
					AfterHandlers.AddHandler("changed_packages", value);
				}
			}
			remove {
				System.ComponentModel.EventHandlerList event_list = AfterHandlers;
				Hashtable signals = AfterSignals;
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					event_list = BeforeHandlers;
					signals = BeforeSignals;
				}
				GLib.SignalCallback cb = signals ["changed_packages"] as GLib.SignalCallback;
				event_list.RemoveHandler("changed_packages", value);
				if (cb == null)
					return;

				cb.RemoveDelegate (value);

				if (event_list["changed_packages"] == null) {
					signals.Remove("changed_packages");
					cb.Dispose ();
				}
			}
		}

		delegate void RefreshedDelegate (IntPtr arg1);

		static RefreshedDelegate RefreshedCallback;

		static void refreshed_cb (IntPtr arg1)
		{
			World obj = GLib.Object.GetObject (arg1, false) as World;
			obj.OnRefreshed ();
		}

		private static void OverrideRefreshed (GLib.GType gtype)
		{
			if (RefreshedCallback == null)
				RefreshedCallback = new RefreshedDelegate (refreshed_cb);
			OverrideVirtualMethod (gtype, "refreshed", RefreshedCallback);
		}

		[GLib.DefaultSignalHandler(Type=typeof(RC.World), ConnectionMethod="OverrideRefreshed")]
		protected virtual void OnRefreshed ()
		{
			GLib.Value ret = GLib.Value.Empty;
			GLib.ValueArray inst_and_params = new GLib.ValueArray (1);
			GLib.Value[] vals = new GLib.Value [1];
			vals [0] = new GLib.Value (this);
			inst_and_params.Append (vals [0]);
			g_signal_chain_from_overridden (inst_and_params.ArrayPtr, ref ret);
		}

		[GLib.Signal("refreshed")]
		public event System.EventHandler Refreshed {
			add {
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					if (BeforeHandlers["refreshed"] == null)
						BeforeSignals["refreshed"] = new RCSharp.voidObjectSignal(this, "refreshed", value, typeof (System.EventArgs), 0);
					else
						((GLib.SignalCallback) BeforeSignals ["refreshed"]).AddDelegate (value);
					BeforeHandlers.AddHandler("refreshed", value);
				} else {
					if (AfterHandlers["refreshed"] == null)
						AfterSignals["refreshed"] = new RCSharp.voidObjectSignal(this, "refreshed", value, typeof (System.EventArgs), 1);
					else
						((GLib.SignalCallback) AfterSignals ["refreshed"]).AddDelegate (value);
					AfterHandlers.AddHandler("refreshed", value);
				}
			}
			remove {
				System.ComponentModel.EventHandlerList event_list = AfterHandlers;
				Hashtable signals = AfterSignals;
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					event_list = BeforeHandlers;
					signals = BeforeSignals;
				}
				GLib.SignalCallback cb = signals ["refreshed"] as GLib.SignalCallback;
				event_list.RemoveHandler("refreshed", value);
				if (cb == null)
					return;

				cb.RemoveDelegate (value);

				if (event_list["refreshed"] == null) {
					signals.Remove("refreshed");
					cb.Dispose ();
				}
			}
		}

		[DllImport("libredcarpet")]
		static extern int rc_world_foreach_requiring_package(IntPtr raw, IntPtr dep, RCSharp.PackageAndDepDelegateNative fn, IntPtr user_data);

		public int ForeachRequiringPackage(RC.PackageDep dep, RC.PackageAndDepDelegate fn) {
			RCSharp.PackageAndDepDelegateWrapper fn_wrapper = null;
			fn_wrapper = new RCSharp.PackageAndDepDelegateWrapper (fn, this);
			int raw_ret = rc_world_foreach_requiring_package(Handle, dep.Handle, fn_wrapper.NativeDelegate, IntPtr.Zero);
			int ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern bool rc_world_package_is_locked(IntPtr raw, IntPtr package);

		public bool PackageIsLocked(RC.Package package) {
			bool raw_ret = rc_world_package_is_locked(Handle, package.Handle);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_get_type();

		public static new GLib.GType GType { 
			get {
				IntPtr raw_ret = rc_world_get_type();
				GLib.GType ret = new GLib.GType(raw_ret);
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_get_channel_by_name(IntPtr raw, string channel_name);

		public RC.Channel GetChannelByName(string channel_name) {
			IntPtr raw_ret = rc_world_get_channel_by_name(Handle, channel_name);
			RC.Channel ret;
			if (raw_ret == IntPtr.Zero)
				ret = null;
			else
				ret = new RC.Channel(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern int rc_world_foreach_lock(IntPtr raw, RCSharp.PackageMatchDelegateNative fn, IntPtr user_data);

		public int ForeachLock(RC.PackageMatchDelegate fn) {
			RCSharp.PackageMatchDelegateWrapper fn_wrapper = null;
			fn_wrapper = new RCSharp.PackageMatchDelegateWrapper (fn, this);
			int raw_ret = rc_world_foreach_lock(Handle, fn_wrapper.NativeDelegate, IntPtr.Zero);
			int ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern uint rc_world_get_channel_sequence_number(IntPtr raw);

		public uint ChannelSequenceNumber { 
			get {
				uint raw_ret = rc_world_get_channel_sequence_number(Handle);
				uint ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_guess_package_channel(IntPtr raw, IntPtr package);

		public RC.Channel GuessPackageChannel(RC.Package package) {
			IntPtr raw_ret = rc_world_guess_package_channel(Handle, package.Handle);
			RC.Channel ret;
			if (raw_ret == IntPtr.Zero)
				ret = null;
			else
				ret = new RC.Channel(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_get_upgrades(IntPtr raw, IntPtr package, IntPtr channel);

		public GLib.SList GetUpgrades(RC.Package package, RC.Channel channel) {
			IntPtr raw_ret = rc_world_get_upgrades(Handle, package.Handle, channel.Handle);
			GLib.SList ret = new GLib.SList(raw_ret, typeof (RC.Package));
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern bool rc_world_is_subscribed(IntPtr raw, IntPtr channel);

		public bool IsSubscribed(RC.Channel channel) {
			bool raw_ret = rc_world_is_subscribed(Handle, channel.Handle);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_refresh_complete(IntPtr raw);

		public void RefreshComplete() {
			rc_world_refresh_complete(Handle);
		}

		[DllImport("libredcarpet")]
		static extern int rc_world_foreach_channel(IntPtr raw, RCSharp.ChannelDelegateNative fn, IntPtr user_data);

		public int ForeachChannel(RC.ChannelDelegate fn) {
			RCSharp.ChannelDelegateWrapper fn_wrapper = null;
			fn_wrapper = new RCSharp.ChannelDelegateWrapper (fn, this);
			int raw_ret = rc_world_foreach_channel(Handle, fn_wrapper.NativeDelegate, IntPtr.Zero);
			int ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern bool rc_world_has_refresh(IntPtr raw);

		public bool HasRefresh { 
			get {
				bool raw_ret = rc_world_has_refresh(Handle);
				bool ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern int rc_world_foreach_providing_package(IntPtr raw, IntPtr dep, RCSharp.PackageAndSpecDelegateNative fn, IntPtr user_data);

		public int ForeachProvidingPackage(RC.PackageDep dep, RC.PackageAndSpecDelegate fn) {
			RCSharp.PackageAndSpecDelegateWrapper fn_wrapper = null;
			fn_wrapper = new RCSharp.PackageAndSpecDelegateWrapper (fn, this);
			int raw_ret = rc_world_foreach_providing_package(Handle, dep.Handle, fn_wrapper.NativeDelegate, IntPtr.Zero);
			int ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_get_channel_by_alias(IntPtr raw, string alias);

		public RC.Channel GetChannelByAlias(string alias) {
			IntPtr raw_ret = rc_world_get_channel_by_alias(Handle, alias);
			RC.Channel ret;
			if (raw_ret == IntPtr.Zero)
				ret = null;
			else
				ret = new RC.Channel(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_get_package_with_constraint(IntPtr raw, IntPtr channel, string name, IntPtr constraint, bool is_and);

		public RC.Package GetPackageWithConstraint(RC.Channel channel, string name, RC.PackageDep constraint, bool is_and) {
			IntPtr raw_ret = rc_world_get_package_with_constraint(Handle, channel.Handle, name, constraint.Handle, is_and);
			RC.Package ret;
			if (raw_ret == IntPtr.Zero)
				ret = null;
			else
				ret = new RC.Package(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_get_package(IntPtr raw, IntPtr channel, string name);

		public RC.Package GetPackage(RC.Channel channel, string name) {
			IntPtr raw_ret = rc_world_get_package(Handle, channel.Handle, name);
			RC.Package ret;
			if (raw_ret == IntPtr.Zero)
				ret = null;
			else
				ret = new RC.Package(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern int rc_world_foreach_package(IntPtr raw, IntPtr channel, RCSharp.PackageDelegateNative fn, IntPtr user_data);

		public int ForeachPackage(RC.Channel channel, RC.PackageDelegate fn) {
			RCSharp.PackageDelegateWrapper fn_wrapper = null;
			fn_wrapper = new RCSharp.PackageDelegateWrapper (fn, this);
			int raw_ret = rc_world_foreach_package(Handle, channel.Handle, fn_wrapper.NativeDelegate, IntPtr.Zero);
			int ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern int rc_world_foreach_system_upgrade(IntPtr raw, bool subscribed_only, RCSharp.PackagePairDelegateNative fn, IntPtr user_data);

		public int ForeachSystemUpgrade(bool subscribed_only, RC.PackagePairDelegate fn) {
			RCSharp.PackagePairDelegateWrapper fn_wrapper = null;
			fn_wrapper = new RCSharp.PackagePairDelegateWrapper (fn, this);
			int raw_ret = rc_world_foreach_system_upgrade(Handle, subscribed_only, fn_wrapper.NativeDelegate, IntPtr.Zero);
			int ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_add_lock(IntPtr raw, IntPtr _lock);

		public void AddLock(RC.PackageMatch _lock) {
			rc_world_add_lock(Handle, _lock.Handle);
		}

		[DllImport("libredcarpet")]
		static extern bool rc_world_contains_channel(IntPtr raw, IntPtr channel);

		public bool ContainsChannel(RC.Channel channel) {
			bool raw_ret = rc_world_contains_channel(Handle, channel.Handle);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern uint rc_world_get_package_sequence_number(IntPtr raw);

		public uint PackageSequenceNumber { 
			get {
				uint raw_ret = rc_world_get_package_sequence_number(Handle);
				uint ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_touch_channel_sequence_number(IntPtr raw);

		public void TouchChannelSequenceNumber() {
			rc_world_touch_channel_sequence_number(Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_remove_lock(IntPtr raw, IntPtr _lock);

		public void RemoveLock(RC.PackageMatch _lock) {
			rc_world_remove_lock(Handle, _lock.Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_set_subscription(IntPtr raw, IntPtr channel, bool is_subscribed);

		public void SetSubscription(RC.Channel channel, bool is_subscribed) {
			rc_world_set_subscription(Handle, channel.Handle, is_subscribed);
		}

		[DllImport("libredcarpet")]
		static extern int rc_world_foreach_package_by_match(IntPtr raw, IntPtr match, RCSharp.PackageDelegateNative fn, IntPtr user_data);

		public int ForeachPackageByMatch(RC.PackageMatch match, RC.PackageDelegate fn) {
			RCSharp.PackageDelegateWrapper fn_wrapper = null;
			fn_wrapper = new RCSharp.PackageDelegateWrapper (fn, this);
			int raw_ret = rc_world_foreach_package_by_match(Handle, match.Handle, fn_wrapper.NativeDelegate, IntPtr.Zero);
			int ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_touch_lock_sequence_number(IntPtr raw);

		public void TouchLockSequenceNumber() {
			rc_world_touch_lock_sequence_number(Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_clear_locks(IntPtr raw);

		public void ClearLocks() {
			rc_world_clear_locks(Handle);
		}

		[DllImport("libredcarpet")]
		static extern bool rc_world_transact(IntPtr raw, IntPtr install_packages, IntPtr remove_packages, int flags);

		public bool Transact(GLib.SList install_packages, GLib.SList remove_packages, int flags) {
			bool raw_ret = rc_world_transact(Handle, install_packages.Handle, remove_packages.Handle, flags);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_refresh(IntPtr raw);

		public RC.Pending Refresh() {
			IntPtr raw_ret = rc_world_refresh(Handle);
			RC.Pending ret;
			if (raw_ret == IntPtr.Zero)
				ret = null;
			else
				ret = (RC.Pending) GLib.Object.GetObject(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_get_locks(IntPtr raw);

		public GLib.SList Locks { 
			get {
				IntPtr raw_ret = rc_world_get_locks(Handle);
				GLib.SList ret = new GLib.SList(raw_ret, typeof (RC.PackageMatch));
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_get_channels(IntPtr raw);

		public GLib.SList Channels { 
			get {
				IntPtr raw_ret = rc_world_get_channels(Handle);
				GLib.SList ret = new GLib.SList(raw_ret, typeof (RC.Channel));
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern int rc_world_foreach_upgrade(IntPtr raw, IntPtr package, IntPtr channel, RCSharp.PackageDelegateNative fn, IntPtr user_data);

		public int ForeachUpgrade(RC.Package package, RC.Channel channel, RC.PackageDelegate fn) {
			RCSharp.PackageDelegateWrapper fn_wrapper = null;
			fn_wrapper = new RCSharp.PackageDelegateWrapper (fn, this);
			int raw_ret = rc_world_foreach_upgrade(Handle, package.Handle, channel.Handle, fn_wrapper.NativeDelegate, IntPtr.Zero);
			int ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern bool rc_world_is_refreshing(IntPtr raw);

		public bool IsRefreshing { 
			get {
				bool raw_ret = rc_world_is_refreshing(Handle);
				bool ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_touch_package_sequence_number(IntPtr raw);

		public void TouchPackageSequenceNumber() {
			rc_world_touch_package_sequence_number(Handle);
		}

		[DllImport("libredcarpet")]
		static extern int rc_world_foreach_conflicting_package(IntPtr raw, IntPtr dep, RCSharp.PackageAndDepDelegateNative fn, IntPtr user_data);

		public int ForeachConflictingPackage(RC.PackageDep dep, RC.PackageAndDepDelegate fn) {
			RCSharp.PackageAndDepDelegateWrapper fn_wrapper = null;
			fn_wrapper = new RCSharp.PackageAndDepDelegateWrapper (fn, this);
			int raw_ret = rc_world_foreach_conflicting_package(Handle, dep.Handle, fn_wrapper.NativeDelegate, IntPtr.Zero);
			int ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern uint rc_world_get_lock_sequence_number(IntPtr raw);

		public uint LockSequenceNumber { 
			get {
				uint raw_ret = rc_world_get_lock_sequence_number(Handle);
				uint ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_find_installed_version(IntPtr raw, IntPtr package);

		public RC.Package FindInstalledVersion(RC.Package package) {
			IntPtr raw_ret = rc_world_find_installed_version(Handle, package.Handle);
			RC.Package ret;
			if (raw_ret == IntPtr.Zero)
				ret = null;
			else
				ret = new RC.Package(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern int rc_world_foreach_parent_package(IntPtr raw, IntPtr dep, RCSharp.PackageAndDepDelegateNative fn, IntPtr user_data);

		public int ForeachParentPackage(RC.PackageDep dep, RC.PackageAndDepDelegate fn) {
			RCSharp.PackageAndDepDelegateWrapper fn_wrapper = null;
			fn_wrapper = new RCSharp.PackageAndDepDelegateWrapper (fn, this);
			int raw_ret = rc_world_foreach_parent_package(Handle, dep.Handle, fn_wrapper.NativeDelegate, IntPtr.Zero);
			int ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern uint rc_world_get_subscription_sequence_number(IntPtr raw);

		public uint SubscriptionSequenceNumber { 
			get {
				uint raw_ret = rc_world_get_subscription_sequence_number(Handle);
				uint ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern int rc_world_foreach_package_by_name(IntPtr raw, string name, IntPtr channel, RCSharp.PackageDelegateNative fn, IntPtr user_data);

		public int ForeachPackageByName(string name, RC.Channel channel, RC.PackageDelegate fn) {
			RCSharp.PackageDelegateWrapper fn_wrapper = null;
			fn_wrapper = new RCSharp.PackageDelegateWrapper (fn, this);
			int raw_ret = rc_world_foreach_package_by_name(Handle, name, channel.Handle, fn_wrapper.NativeDelegate, IntPtr.Zero);
			int ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern bool rc_world_sync_conditional(IntPtr raw, IntPtr arg1);

		public bool SyncConditional(RC.Channel arg1) {
			bool raw_ret = rc_world_sync_conditional(Handle, arg1.Handle);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_dup(IntPtr raw);

		public RC.World Dup() {
			IntPtr raw_ret = rc_world_dup(Handle);
			RC.World ret;
			if (raw_ret == IntPtr.Zero)
				ret = null;
			else
				ret = (RC.World) GLib.Object.GetObject(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_get_channel_by_id(IntPtr raw, string channel_id);

		public RC.Channel GetChannelById(string channel_id) {
			IntPtr raw_ret = rc_world_get_channel_by_id(Handle, channel_id);
			RC.Channel ret;
			if (raw_ret == IntPtr.Zero)
				ret = null;
			else
				ret = new RC.Channel(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern bool rc_world_sync(IntPtr raw);

		public bool Sync() {
			bool raw_ret = rc_world_sync(Handle);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_touch_subscription_sequence_number(IntPtr raw);

		public void TouchSubscriptionSequenceNumber() {
			rc_world_touch_subscription_sequence_number(Handle);
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_get_best_upgrade(IntPtr raw, IntPtr package, bool subscribed_only);

		public RC.Package GetBestUpgrade(RC.Package package, bool subscribed_only) {
			IntPtr raw_ret = rc_world_get_best_upgrade(Handle, package.Handle, subscribed_only);
			RC.Package ret;
			if (raw_ret == IntPtr.Zero)
				ret = null;
			else
				ret = new RC.Package(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_refresh_begin(IntPtr raw);

		public void RefreshBegin() {
			rc_world_refresh_begin(Handle);
		}

		[DllImport("libredcarpet")]
		static extern bool rc_world_can_transact_package(IntPtr raw, IntPtr package);

		public bool CanTransactPackage(RC.Package package) {
			bool raw_ret = rc_world_can_transact_package(Handle, package.Handle);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern bool rc_world_get_single_provider(IntPtr raw, IntPtr dep, IntPtr channel, IntPtr package);

		public bool GetSingleProvider(RC.PackageDep dep, RC.Channel channel, RC.Package package) {
			bool raw_ret = rc_world_get_single_provider(Handle, dep.Handle, channel.Handle, package.Handle);
			bool ret = raw_ret;
			return ret;
		}


		static World ()
		{
			GtkSharp.LibredcarpetSharp.ObjectManager.Initialize ();
		}
#endregion
#region Customized extensions
#line 1 "World.custom"
/* -*- Mode: csharp; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// namespace {

    [DllImport("libredcarpet")]
    static extern IntPtr rc_get_world();

    [DllImport("libredcarpet")]
    static extern void rc_set_world(IntPtr world);

    public static RC.World Global { 
        get {
            IntPtr raw_ret = rc_get_world();
            RC.World ret;
            if (raw_ret == IntPtr.Zero)
                ret = null;
            else
                ret = (RC.World) GLib.Object.GetObject(raw_ret);
            return ret;
        }
        set {
            rc_set_world((value == null) ? IntPtr.Zero : value.Handle);
        }
    }

    [DllImport("libredcarpet")]
    static extern void rc_world_set_refresh_function(IntPtr raw, RCSharp.WorldRefreshDelegateNative refresh_fn);

    public RC.WorldRefreshDelegate RefreshDelegate { 
        set {
            RCSharp.WorldRefreshDelegateWrapper value_wrapper = null;
            value_wrapper = new RCSharp.WorldRefreshDelegateWrapper (value, this);
            rc_world_set_refresh_function(Handle, value_wrapper.NativeDelegate);
        }
    }

    private class ToXmlHelper
    {
        private System.Xml.XmlTextWriter writer;
        private World world;

        public ToXmlHelper (World world, System.Xml.XmlTextWriter writer)
        {
            this.writer = writer;
        }

        public bool ForeachPackage (Package package)
        {
            package.ToXml (this.writer);

            return true;
        }

        public bool ForeachChannel (Channel channel)
        {
            /* These are handled by the "system_packages" section */
            if (channel.IsSystem)
                return true;

            channel.ToXml (this.writer);
            return true;
        }

        public bool ForeachLock (PackageMatch match)
        {
            match.ToXml (this.writer);
            return true;
        }
    }

    public void ToXml (System.Xml.XmlTextWriter writer)
    {
        ToXmlHelper helper = new ToXmlHelper (this, writer);

        writer.WriteStartElement ("system_packages");
        this.ForeachPackage (Channel.SYSTEM, new PackageDelegate (helper.ForeachPackage));
        writer.WriteEndElement ();

        this.ForeachChannel (new ChannelDelegate (helper.ForeachChannel));
    }

    public void ToXml (System.IO.Stream stream)
    {
        System.Xml.XmlTextWriter writer = new System.Xml.XmlTextWriter (stream, null);
        writer.WriteStartDocument ();
        writer.WriteStartElement ("world");

        writer.WriteStartElement ("locks");
        ToXmlHelper helper = new ToXmlHelper (this, writer);
        this.ForeachLock (new PackageMatchDelegate (helper.ForeachLock));
        writer.WriteEndElement ();

        this.ToXml (writer);

        writer.WriteEndElement ();
        writer.WriteEndDocument ();
        writer.Flush ();
    }

#endregion
	}
}
