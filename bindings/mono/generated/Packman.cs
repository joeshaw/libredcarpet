// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	public  class Packman : GLib.Object {

		~Packman()
		{
			Dispose();
		}

		[Obsolete]
		protected Packman(GLib.GType gtype) : base(gtype) {}
		public Packman(IntPtr raw) : base(raw) {}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_packman_new();

		public Packman () : base (IntPtr.Zero)
		{
			if (GetType () != typeof (Packman)) {
				CreateNativeObject (new string [0], new GLib.Value[0]);
				return;
			}
			Raw = rc_packman_new();
		}

		delegate void TransactStepDelegate (IntPtr packman, int seqno, int step, string name);

		static TransactStepDelegate TransactStepCallback;

		static void transactstep_cb (IntPtr packman, int seqno, int step, string name)
		{
			Packman obj = GLib.Object.GetObject (packman, false) as Packman;
			obj.OnTransactStep (seqno, (RC.PackmanStep) step, name);
		}

		private static void OverrideTransactStep (GLib.GType gtype)
		{
			if (TransactStepCallback == null)
				TransactStepCallback = new TransactStepDelegate (transactstep_cb);
			OverrideVirtualMethod (gtype, "transact_step", TransactStepCallback);
		}

		[GLib.DefaultSignalHandler(Type=typeof(RC.Packman), ConnectionMethod="OverrideTransactStep")]
		protected virtual void OnTransactStep (int seqno, RC.PackmanStep step, string name)
		{
			GLib.Value ret = GLib.Value.Empty;
			GLib.ValueArray inst_and_params = new GLib.ValueArray (4);
			GLib.Value[] vals = new GLib.Value [4];
			vals [0] = new GLib.Value (this);
			inst_and_params.Append (vals [0]);
			vals [1] = new GLib.Value (seqno);
			inst_and_params.Append (vals [1]);
			vals [2] = new GLib.Value (step);
			inst_and_params.Append (vals [2]);
			vals [3] = new GLib.Value (name);
			inst_and_params.Append (vals [3]);
			g_signal_chain_from_overridden (inst_and_params.ArrayPtr, ref ret);
		}

		[GLib.Signal("transact_step")]
		public event RC.TransactStepHandler TransactStep {
			add {
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					if (BeforeHandlers["transact_step"] == null)
						BeforeSignals["transact_step"] = new RCSharp.voidObjectintPackmanStepstringSignal(this, "transact_step", value, typeof (RC.TransactStepArgs), 0);
					else
						((GLib.SignalCallback) BeforeSignals ["transact_step"]).AddDelegate (value);
					BeforeHandlers.AddHandler("transact_step", value);
				} else {
					if (AfterHandlers["transact_step"] == null)
						AfterSignals["transact_step"] = new RCSharp.voidObjectintPackmanStepstringSignal(this, "transact_step", value, typeof (RC.TransactStepArgs), 1);
					else
						((GLib.SignalCallback) AfterSignals ["transact_step"]).AddDelegate (value);
					AfterHandlers.AddHandler("transact_step", value);
				}
			}
			remove {
				System.ComponentModel.EventHandlerList event_list = AfterHandlers;
				Hashtable signals = AfterSignals;
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					event_list = BeforeHandlers;
					signals = BeforeSignals;
				}
				GLib.SignalCallback cb = signals ["transact_step"] as GLib.SignalCallback;
				event_list.RemoveHandler("transact_step", value);
				if (cb == null)
					return;

				cb.RemoveDelegate (value);

				if (event_list["transact_step"] == null) {
					signals.Remove("transact_step");
					cb.Dispose ();
				}
			}
		}

		delegate void TransactDoneDelegate (IntPtr packman);

		static TransactDoneDelegate TransactDoneCallback;

		static void transactdone_cb (IntPtr packman)
		{
			Packman obj = GLib.Object.GetObject (packman, false) as Packman;
			obj.OnTransactDone ();
		}

		private static void OverrideTransactDone (GLib.GType gtype)
		{
			if (TransactDoneCallback == null)
				TransactDoneCallback = new TransactDoneDelegate (transactdone_cb);
			OverrideVirtualMethod (gtype, "transact_done", TransactDoneCallback);
		}

		[GLib.DefaultSignalHandler(Type=typeof(RC.Packman), ConnectionMethod="OverrideTransactDone")]
		protected virtual void OnTransactDone ()
		{
			GLib.Value ret = GLib.Value.Empty;
			GLib.ValueArray inst_and_params = new GLib.ValueArray (1);
			GLib.Value[] vals = new GLib.Value [1];
			vals [0] = new GLib.Value (this);
			inst_and_params.Append (vals [0]);
			g_signal_chain_from_overridden (inst_and_params.ArrayPtr, ref ret);
		}

		[GLib.Signal("transact_done")]
		public event System.EventHandler TransactDone {
			add {
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					if (BeforeHandlers["transact_done"] == null)
						BeforeSignals["transact_done"] = new RCSharp.voidObjectSignal(this, "transact_done", value, typeof (System.EventArgs), 0);
					else
						((GLib.SignalCallback) BeforeSignals ["transact_done"]).AddDelegate (value);
					BeforeHandlers.AddHandler("transact_done", value);
				} else {
					if (AfterHandlers["transact_done"] == null)
						AfterSignals["transact_done"] = new RCSharp.voidObjectSignal(this, "transact_done", value, typeof (System.EventArgs), 1);
					else
						((GLib.SignalCallback) AfterSignals ["transact_done"]).AddDelegate (value);
					AfterHandlers.AddHandler("transact_done", value);
				}
			}
			remove {
				System.ComponentModel.EventHandlerList event_list = AfterHandlers;
				Hashtable signals = AfterSignals;
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					event_list = BeforeHandlers;
					signals = BeforeSignals;
				}
				GLib.SignalCallback cb = signals ["transact_done"] as GLib.SignalCallback;
				event_list.RemoveHandler("transact_done", value);
				if (cb == null)
					return;

				cb.RemoveDelegate (value);

				if (event_list["transact_done"] == null) {
					signals.Remove("transact_done");
					cb.Dispose ();
				}
			}
		}

		delegate void DatabaseChangedDelegate (IntPtr packman);

		static DatabaseChangedDelegate DatabaseChangedCallback;

		static void databasechanged_cb (IntPtr packman)
		{
			Packman obj = GLib.Object.GetObject (packman, false) as Packman;
			obj.OnDatabaseChanged ();
		}

		private static void OverrideDatabaseChanged (GLib.GType gtype)
		{
			if (DatabaseChangedCallback == null)
				DatabaseChangedCallback = new DatabaseChangedDelegate (databasechanged_cb);
			OverrideVirtualMethod (gtype, "database_changed", DatabaseChangedCallback);
		}

		[GLib.DefaultSignalHandler(Type=typeof(RC.Packman), ConnectionMethod="OverrideDatabaseChanged")]
		protected virtual void OnDatabaseChanged ()
		{
			GLib.Value ret = GLib.Value.Empty;
			GLib.ValueArray inst_and_params = new GLib.ValueArray (1);
			GLib.Value[] vals = new GLib.Value [1];
			vals [0] = new GLib.Value (this);
			inst_and_params.Append (vals [0]);
			g_signal_chain_from_overridden (inst_and_params.ArrayPtr, ref ret);
		}

		[GLib.Signal("database_changed")]
		public event System.EventHandler DatabaseChanged {
			add {
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					if (BeforeHandlers["database_changed"] == null)
						BeforeSignals["database_changed"] = new RCSharp.voidObjectSignal(this, "database_changed", value, typeof (System.EventArgs), 0);
					else
						((GLib.SignalCallback) BeforeSignals ["database_changed"]).AddDelegate (value);
					BeforeHandlers.AddHandler("database_changed", value);
				} else {
					if (AfterHandlers["database_changed"] == null)
						AfterSignals["database_changed"] = new RCSharp.voidObjectSignal(this, "database_changed", value, typeof (System.EventArgs), 1);
					else
						((GLib.SignalCallback) AfterSignals ["database_changed"]).AddDelegate (value);
					AfterHandlers.AddHandler("database_changed", value);
				}
			}
			remove {
				System.ComponentModel.EventHandlerList event_list = AfterHandlers;
				Hashtable signals = AfterSignals;
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					event_list = BeforeHandlers;
					signals = BeforeSignals;
				}
				GLib.SignalCallback cb = signals ["database_changed"] as GLib.SignalCallback;
				event_list.RemoveHandler("database_changed", value);
				if (cb == null)
					return;

				cb.RemoveDelegate (value);

				if (event_list["database_changed"] == null) {
					signals.Remove("database_changed");
					cb.Dispose ();
				}
			}
		}

		delegate void TransactStartDelegate (IntPtr packman, int total_steps);

		static TransactStartDelegate TransactStartCallback;

		static void transactstart_cb (IntPtr packman, int total_steps)
		{
			Packman obj = GLib.Object.GetObject (packman, false) as Packman;
			obj.OnTransactStart (total_steps);
		}

		private static void OverrideTransactStart (GLib.GType gtype)
		{
			if (TransactStartCallback == null)
				TransactStartCallback = new TransactStartDelegate (transactstart_cb);
			OverrideVirtualMethod (gtype, "transact_start", TransactStartCallback);
		}

		[GLib.DefaultSignalHandler(Type=typeof(RC.Packman), ConnectionMethod="OverrideTransactStart")]
		protected virtual void OnTransactStart (int total_steps)
		{
			GLib.Value ret = GLib.Value.Empty;
			GLib.ValueArray inst_and_params = new GLib.ValueArray (2);
			GLib.Value[] vals = new GLib.Value [2];
			vals [0] = new GLib.Value (this);
			inst_and_params.Append (vals [0]);
			vals [1] = new GLib.Value (total_steps);
			inst_and_params.Append (vals [1]);
			g_signal_chain_from_overridden (inst_and_params.ArrayPtr, ref ret);
		}

		[GLib.Signal("transact_start")]
		public event RC.TransactStartHandler TransactStart {
			add {
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					if (BeforeHandlers["transact_start"] == null)
						BeforeSignals["transact_start"] = new RCSharp.voidObjectintSignal(this, "transact_start", value, typeof (RC.TransactStartArgs), 0);
					else
						((GLib.SignalCallback) BeforeSignals ["transact_start"]).AddDelegate (value);
					BeforeHandlers.AddHandler("transact_start", value);
				} else {
					if (AfterHandlers["transact_start"] == null)
						AfterSignals["transact_start"] = new RCSharp.voidObjectintSignal(this, "transact_start", value, typeof (RC.TransactStartArgs), 1);
					else
						((GLib.SignalCallback) AfterSignals ["transact_start"]).AddDelegate (value);
					AfterHandlers.AddHandler("transact_start", value);
				}
			}
			remove {
				System.ComponentModel.EventHandlerList event_list = AfterHandlers;
				Hashtable signals = AfterSignals;
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					event_list = BeforeHandlers;
					signals = BeforeSignals;
				}
				GLib.SignalCallback cb = signals ["transact_start"] as GLib.SignalCallback;
				event_list.RemoveHandler("transact_start", value);
				if (cb == null)
					return;

				cb.RemoveDelegate (value);

				if (event_list["transact_start"] == null) {
					signals.Remove("transact_start");
					cb.Dispose ();
				}
			}
		}

		delegate void DatabaseLockedDelegate (IntPtr packman);

		static DatabaseLockedDelegate DatabaseLockedCallback;

		static void databaselocked_cb (IntPtr packman)
		{
			Packman obj = GLib.Object.GetObject (packman, false) as Packman;
			obj.OnDatabaseLocked ();
		}

		private static void OverrideDatabaseLocked (GLib.GType gtype)
		{
			if (DatabaseLockedCallback == null)
				DatabaseLockedCallback = new DatabaseLockedDelegate (databaselocked_cb);
			OverrideVirtualMethod (gtype, "database_locked", DatabaseLockedCallback);
		}

		[GLib.DefaultSignalHandler(Type=typeof(RC.Packman), ConnectionMethod="OverrideDatabaseLocked")]
		protected virtual void OnDatabaseLocked ()
		{
			GLib.Value ret = GLib.Value.Empty;
			GLib.ValueArray inst_and_params = new GLib.ValueArray (1);
			GLib.Value[] vals = new GLib.Value [1];
			vals [0] = new GLib.Value (this);
			inst_and_params.Append (vals [0]);
			g_signal_chain_from_overridden (inst_and_params.ArrayPtr, ref ret);
		}

		[GLib.Signal("database_locked")]
		public event System.EventHandler DatabaseLocked {
			add {
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					if (BeforeHandlers["database_locked"] == null)
						BeforeSignals["database_locked"] = new RCSharp.voidObjectSignal(this, "database_locked", value, typeof (System.EventArgs), 0);
					else
						((GLib.SignalCallback) BeforeSignals ["database_locked"]).AddDelegate (value);
					BeforeHandlers.AddHandler("database_locked", value);
				} else {
					if (AfterHandlers["database_locked"] == null)
						AfterSignals["database_locked"] = new RCSharp.voidObjectSignal(this, "database_locked", value, typeof (System.EventArgs), 1);
					else
						((GLib.SignalCallback) AfterSignals ["database_locked"]).AddDelegate (value);
					AfterHandlers.AddHandler("database_locked", value);
				}
			}
			remove {
				System.ComponentModel.EventHandlerList event_list = AfterHandlers;
				Hashtable signals = AfterSignals;
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					event_list = BeforeHandlers;
					signals = BeforeSignals;
				}
				GLib.SignalCallback cb = signals ["database_locked"] as GLib.SignalCallback;
				event_list.RemoveHandler("database_locked", value);
				if (cb == null)
					return;

				cb.RemoveDelegate (value);

				if (event_list["database_locked"] == null) {
					signals.Remove("database_locked");
					cb.Dispose ();
				}
			}
		}

		delegate void TransactProgressDelegate (IntPtr packman, UIntPtr amount, UIntPtr total);

		static TransactProgressDelegate TransactProgressCallback;

		static void transactprogress_cb (IntPtr packman, UIntPtr amount, UIntPtr total)
		{
			Packman obj = GLib.Object.GetObject (packman, false) as Packman;
			obj.OnTransactProgress ((ulong) amount, (ulong) total);
		}

		private static void OverrideTransactProgress (GLib.GType gtype)
		{
			if (TransactProgressCallback == null)
				TransactProgressCallback = new TransactProgressDelegate (transactprogress_cb);
			OverrideVirtualMethod (gtype, "transact_progress", TransactProgressCallback);
		}

		[GLib.DefaultSignalHandler(Type=typeof(RC.Packman), ConnectionMethod="OverrideTransactProgress")]
		protected virtual void OnTransactProgress (ulong amount, ulong total)
		{
			GLib.Value ret = GLib.Value.Empty;
			GLib.ValueArray inst_and_params = new GLib.ValueArray (3);
			GLib.Value[] vals = new GLib.Value [3];
			vals [0] = new GLib.Value (this);
			inst_and_params.Append (vals [0]);
			vals [1] = new GLib.Value (amount);
			inst_and_params.Append (vals [1]);
			vals [2] = new GLib.Value (total);
			inst_and_params.Append (vals [2]);
			g_signal_chain_from_overridden (inst_and_params.ArrayPtr, ref ret);
		}

		[GLib.Signal("transact_progress")]
		public event RC.TransactProgressHandler TransactProgress {
			add {
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					if (BeforeHandlers["transact_progress"] == null)
						BeforeSignals["transact_progress"] = new RCSharp.voidObjectulongulongSignal(this, "transact_progress", value, typeof (RC.TransactProgressArgs), 0);
					else
						((GLib.SignalCallback) BeforeSignals ["transact_progress"]).AddDelegate (value);
					BeforeHandlers.AddHandler("transact_progress", value);
				} else {
					if (AfterHandlers["transact_progress"] == null)
						AfterSignals["transact_progress"] = new RCSharp.voidObjectulongulongSignal(this, "transact_progress", value, typeof (RC.TransactProgressArgs), 1);
					else
						((GLib.SignalCallback) AfterSignals ["transact_progress"]).AddDelegate (value);
					AfterHandlers.AddHandler("transact_progress", value);
				}
			}
			remove {
				System.ComponentModel.EventHandlerList event_list = AfterHandlers;
				Hashtable signals = AfterSignals;
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					event_list = BeforeHandlers;
					signals = BeforeSignals;
				}
				GLib.SignalCallback cb = signals ["transact_progress"] as GLib.SignalCallback;
				event_list.RemoveHandler("transact_progress", value);
				if (cb == null)
					return;

				cb.RemoveDelegate (value);

				if (event_list["transact_progress"] == null) {
					signals.Remove("transact_progress");
					cb.Dispose ();
				}
			}
		}

		delegate void DatabaseUnlockedDelegate (IntPtr packman);

		static DatabaseUnlockedDelegate DatabaseUnlockedCallback;

		static void databaseunlocked_cb (IntPtr packman)
		{
			Packman obj = GLib.Object.GetObject (packman, false) as Packman;
			obj.OnDatabaseUnlocked ();
		}

		private static void OverrideDatabaseUnlocked (GLib.GType gtype)
		{
			if (DatabaseUnlockedCallback == null)
				DatabaseUnlockedCallback = new DatabaseUnlockedDelegate (databaseunlocked_cb);
			OverrideVirtualMethod (gtype, "database_unlocked", DatabaseUnlockedCallback);
		}

		[GLib.DefaultSignalHandler(Type=typeof(RC.Packman), ConnectionMethod="OverrideDatabaseUnlocked")]
		protected virtual void OnDatabaseUnlocked ()
		{
			GLib.Value ret = GLib.Value.Empty;
			GLib.ValueArray inst_and_params = new GLib.ValueArray (1);
			GLib.Value[] vals = new GLib.Value [1];
			vals [0] = new GLib.Value (this);
			inst_and_params.Append (vals [0]);
			g_signal_chain_from_overridden (inst_and_params.ArrayPtr, ref ret);
		}

		[GLib.Signal("database_unlocked")]
		public event System.EventHandler DatabaseUnlocked {
			add {
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					if (BeforeHandlers["database_unlocked"] == null)
						BeforeSignals["database_unlocked"] = new RCSharp.voidObjectSignal(this, "database_unlocked", value, typeof (System.EventArgs), 0);
					else
						((GLib.SignalCallback) BeforeSignals ["database_unlocked"]).AddDelegate (value);
					BeforeHandlers.AddHandler("database_unlocked", value);
				} else {
					if (AfterHandlers["database_unlocked"] == null)
						AfterSignals["database_unlocked"] = new RCSharp.voidObjectSignal(this, "database_unlocked", value, typeof (System.EventArgs), 1);
					else
						((GLib.SignalCallback) AfterSignals ["database_unlocked"]).AddDelegate (value);
					AfterHandlers.AddHandler("database_unlocked", value);
				}
			}
			remove {
				System.ComponentModel.EventHandlerList event_list = AfterHandlers;
				Hashtable signals = AfterSignals;
				if (value.Method.GetCustomAttributes(typeof(GLib.ConnectBeforeAttribute), false).Length > 0) {
					event_list = BeforeHandlers;
					signals = BeforeSignals;
				}
				GLib.SignalCallback cb = signals ["database_unlocked"] as GLib.SignalCallback;
				event_list.RemoveHandler("database_unlocked", value);
				if (cb == null)
					return;

				cb.RemoveDelegate (value);

				if (event_list["database_unlocked"] == null) {
					signals.Remove("database_unlocked");
					cb.Dispose ();
				}
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_packman_verify(IntPtr raw, IntPtr package, uint type);

		public GLib.SList Verify(RC.Package package, uint type) {
			IntPtr raw_ret = rc_packman_verify(Handle, package.Handle, type);
			GLib.SList ret = new GLib.SList(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_packman_patch_parents(IntPtr raw, IntPtr package);

		public GLib.SList PatchParents(RC.Package package) {
			IntPtr raw_ret = rc_packman_patch_parents(Handle, package.Handle);
			GLib.SList ret = new GLib.SList(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern int rc_packman_version_compare(IntPtr raw, IntPtr spec1, IntPtr spec2);

		public int VersionCompare(RC.PackageSpec spec1, RC.PackageSpec spec2) {
			int raw_ret = rc_packman_version_compare(Handle, spec1.Handle, spec2.Handle);
			int ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern bool rc_packman_lock(IntPtr raw);

		public bool Lock() {
			bool raw_ret = rc_packman_lock(Handle);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_packman_file_list(IntPtr raw, IntPtr package);

		public GLib.SList FileList(RC.Package package) {
			IntPtr raw_ret = rc_packman_file_list(Handle, package.Handle);
			GLib.SList ret = new GLib.SList(raw_ret, typeof (RC.PackageFile));
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_packman_query(IntPtr raw, string name);

		public GLib.SList Query(string name) {
			IntPtr raw_ret = rc_packman_query(Handle, name);
			GLib.SList ret = new GLib.SList(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern uint rc_packman_get_error(IntPtr raw);

		public uint Error { 
			get {
				uint raw_ret = rc_packman_get_error(Handle);
				uint ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_packman_query_file_list(IntPtr raw, IntPtr filenames, bool filter_file_deps);

		public GLib.SList QueryFileList(GLib.SList filenames, bool filter_file_deps) {
			IntPtr raw_ret = rc_packman_query_file_list(Handle, filenames.Handle, filter_file_deps);
			GLib.SList ret = new GLib.SList(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_packman_transact(IntPtr raw, IntPtr install_packages, IntPtr remove_packages, int flags);

		public void Transact(GLib.SList install_packages, GLib.SList remove_packages, int flags) {
			rc_packman_transact(Handle, install_packages.Handle, remove_packages.Handle, flags);
		}

		[DllImport("libredcarpet")]
		static extern bool rc_packman_is_locked(IntPtr raw);

		public bool IsLocked { 
			get {
				bool raw_ret = rc_packman_is_locked(Handle);
				bool ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern bool rc_packman_get_rollback_enabled(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_packman_set_rollback_enabled(IntPtr raw, bool enabled);

		public bool RollbackEnabled { 
			get {
				bool raw_ret = rc_packman_get_rollback_enabled(Handle);
				bool ret = raw_ret;
				return ret;
			}
			set {
				rc_packman_set_rollback_enabled(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern bool rc_packman_is_database_changed(IntPtr raw);

		public bool IsDatabaseChanged { 
			get {
				bool raw_ret = rc_packman_is_database_changed(Handle);
				bool ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_packman_query_all(IntPtr raw);

		public GLib.SList QueryAll() {
			IntPtr raw_ret = rc_packman_query_all(Handle);
			GLib.SList ret = new GLib.SList(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_packman_find_file(IntPtr raw, string filename);

		public GLib.SList FindFile(string filename) {
			IntPtr raw_ret = rc_packman_find_file(Handle, filename);
			GLib.SList ret = new GLib.SList(raw_ret, typeof (RC.Package));
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_packman_get_file_extension(IntPtr raw);

		public string FileExtension { 
			get {
				IntPtr raw_ret = rc_packman_get_file_extension(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern uint rc_packman_get_capabilities(IntPtr raw);

		public uint Capabilities { 
			get {
				uint raw_ret = rc_packman_get_capabilities(Handle);
				uint ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern bool rc_packman_parse_version(IntPtr raw, string input, out bool has_epoch, out uint epoch, out string version, out string release);

		public bool ParseVersion(string input, out bool has_epoch, out uint epoch, out string version, out string release) {
			bool raw_ret = rc_packman_parse_version(Handle, input, out has_epoch, out epoch, out version, out release);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_packman_get_reason(IntPtr raw);

		public string Reason { 
			get {
				IntPtr raw_ret = rc_packman_get_reason(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_packman_get_type();

		public static new GLib.GType GType { 
			get {
				IntPtr raw_ret = rc_packman_get_type();
				GLib.GType ret = new GLib.GType(raw_ret);
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_packman_unlock(IntPtr raw);

		public void Unlock() {
			rc_packman_unlock(Handle);
		}


		static Packman ()
		{
			GtkSharp.LibredcarpetSharp.ObjectManager.Initialize ();
		}
#endregion
#region Customized extensions
#line 1 "Packman.custom"
/* -*- Mode: csharp; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// namespace {
    public static uint CapabilityNone                = 0;
    public static uint CapabilityProvideAllVersions  = (1 << 0);
    public static uint CapabilityIgnoreAbsentEpochs  = (1 << 2);
    public static uint CapabilityRollback            = (1 << 3);
    public static uint CapabilityAlwaysVerifyRelease = (1 << 4);
    public static uint CapabilityPatches             = (1 << 5);

    [DllImport("libredcarpet")]
    static extern IntPtr rc_packman_query_file(IntPtr raw, string filename, bool filter_file_deps);

    public RC.Package QueryFile(string filename, bool filter_file_deps) {
        IntPtr raw_ret = rc_packman_query_file(Handle, filename, filter_file_deps);
        RC.Package ret;
        if (raw_ret == IntPtr.Zero)
            ret = null;
        else
            ret = new RC.Package(raw_ret, true);
        return ret;
    }

    [DllImport("libredcarpet")]
    static extern void rc_packman_set_global(IntPtr raw);

    [DllImport("libredcarpet")]
    static extern IntPtr rc_packman_get_global();

    public static RC.Packman Global { 
        get {
            IntPtr raw_ret = rc_packman_get_global();
            RC.Packman ret;
            if (raw_ret == IntPtr.Zero)
                ret = null;
            else
                ret = (RC.Packman) GLib.Object.GetObject(raw_ret);
            return ret;
        }

        set {
            rc_packman_set_global ((value == null) ? IntPtr.Zero : value.Handle);
        }
    }

    [DllImport("libredcarpet")]
    static extern IntPtr rc_distman_new();

    public static RC.Packman Distman() {
        IntPtr raw_ret = rc_distman_new();
        RC.Packman ret;
        if (raw_ret == IntPtr.Zero)
            ret = null;
        else
            ret = (RC.Packman) GLib.Object.GetObject(raw_ret, true);
        return ret;
    }

#endregion
	}
}
