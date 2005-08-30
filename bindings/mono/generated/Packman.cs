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

		[GLib.CDeclCallback]
		delegate void TransactStepSignalDelegate (IntPtr arg0, int arg1, int arg2, IntPtr arg3, IntPtr gch);

		static void TransactStepSignalCallback (IntPtr arg0, int arg1, int arg2, IntPtr arg3, IntPtr gch)
		{
			GLib.Signal sig = ((GCHandle) gch).Target as GLib.Signal;
			if (sig == null)
				throw new Exception("Unknown signal GC handle received " + gch);

			RC.TransactStepArgs args = new RC.TransactStepArgs ();
			args.Args = new object[3];
			args.Args[0] = arg1;
			args.Args[1] = (RC.PackmanStep) arg2;
			args.Args[2] = GLib.Marshaller.PtrToStringGFree(arg3);
			RC.TransactStepHandler handler = (RC.TransactStepHandler) sig.Handler;
			handler (GLib.Object.GetObject (arg0), args);

		}

		[GLib.CDeclCallback]
		delegate void TransactStepVMDelegate (IntPtr packman, int seqno, int step, IntPtr name);

		static TransactStepVMDelegate TransactStepVMCallback;

		static void transactstep_cb (IntPtr packman, int seqno, int step, IntPtr name)
		{
			Packman packman_managed = GLib.Object.GetObject (packman, false) as Packman;
			packman_managed.OnTransactStep (seqno, (RC.PackmanStep) step, GLib.Marshaller.PtrToStringGFree(name));
		}

		private static void OverrideTransactStep (GLib.GType gtype)
		{
			if (TransactStepVMCallback == null)
				TransactStepVMCallback = new TransactStepVMDelegate (transactstep_cb);
			OverrideVirtualMethod (gtype, "transact_step", TransactStepVMCallback);
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
			foreach (GLib.Value v in vals)
				v.Dispose ();
		}

		[GLib.Signal("transact_step")]
		public event RC.TransactStepHandler TransactStep {
			add {
				GLib.Signal sig = GLib.Signal.Lookup (this, "transact_step", new TransactStepSignalDelegate(TransactStepSignalCallback));
				sig.AddDelegate (value);
			}
			remove {
				GLib.Signal sig = GLib.Signal.Lookup (this, "transact_step", new TransactStepSignalDelegate(TransactStepSignalCallback));
				sig.RemoveDelegate (value);
			}
		}

		[GLib.CDeclCallback]
		delegate void TransactDoneVMDelegate (IntPtr packman);

		static TransactDoneVMDelegate TransactDoneVMCallback;

		static void transactdone_cb (IntPtr packman)
		{
			Packman packman_managed = GLib.Object.GetObject (packman, false) as Packman;
			packman_managed.OnTransactDone ();
		}

		private static void OverrideTransactDone (GLib.GType gtype)
		{
			if (TransactDoneVMCallback == null)
				TransactDoneVMCallback = new TransactDoneVMDelegate (transactdone_cb);
			OverrideVirtualMethod (gtype, "transact_done", TransactDoneVMCallback);
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
			foreach (GLib.Value v in vals)
				v.Dispose ();
		}

		[GLib.Signal("transact_done")]
		public event System.EventHandler TransactDone {
			add {
				GLib.Signal sig = GLib.Signal.Lookup (this, "transact_done");
				sig.AddDelegate (value);
			}
			remove {
				GLib.Signal sig = GLib.Signal.Lookup (this, "transact_done");
				sig.RemoveDelegate (value);
			}
		}

		[GLib.CDeclCallback]
		delegate void DatabaseChangedVMDelegate (IntPtr packman);

		static DatabaseChangedVMDelegate DatabaseChangedVMCallback;

		static void databasechanged_cb (IntPtr packman)
		{
			Packman packman_managed = GLib.Object.GetObject (packman, false) as Packman;
			packman_managed.OnDatabaseChanged ();
		}

		private static void OverrideDatabaseChanged (GLib.GType gtype)
		{
			if (DatabaseChangedVMCallback == null)
				DatabaseChangedVMCallback = new DatabaseChangedVMDelegate (databasechanged_cb);
			OverrideVirtualMethod (gtype, "database_changed", DatabaseChangedVMCallback);
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
			foreach (GLib.Value v in vals)
				v.Dispose ();
		}

		[GLib.Signal("database_changed")]
		public event System.EventHandler DatabaseChanged {
			add {
				GLib.Signal sig = GLib.Signal.Lookup (this, "database_changed");
				sig.AddDelegate (value);
			}
			remove {
				GLib.Signal sig = GLib.Signal.Lookup (this, "database_changed");
				sig.RemoveDelegate (value);
			}
		}

		[GLib.CDeclCallback]
		delegate void TransactStartSignalDelegate (IntPtr arg0, int arg1, IntPtr gch);

		static void TransactStartSignalCallback (IntPtr arg0, int arg1, IntPtr gch)
		{
			GLib.Signal sig = ((GCHandle) gch).Target as GLib.Signal;
			if (sig == null)
				throw new Exception("Unknown signal GC handle received " + gch);

			RC.TransactStartArgs args = new RC.TransactStartArgs ();
			args.Args = new object[1];
			args.Args[0] = arg1;
			RC.TransactStartHandler handler = (RC.TransactStartHandler) sig.Handler;
			handler (GLib.Object.GetObject (arg0), args);

		}

		[GLib.CDeclCallback]
		delegate void TransactStartVMDelegate (IntPtr packman, int total_steps);

		static TransactStartVMDelegate TransactStartVMCallback;

		static void transactstart_cb (IntPtr packman, int total_steps)
		{
			Packman packman_managed = GLib.Object.GetObject (packman, false) as Packman;
			packman_managed.OnTransactStart (total_steps);
		}

		private static void OverrideTransactStart (GLib.GType gtype)
		{
			if (TransactStartVMCallback == null)
				TransactStartVMCallback = new TransactStartVMDelegate (transactstart_cb);
			OverrideVirtualMethod (gtype, "transact_start", TransactStartVMCallback);
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
			foreach (GLib.Value v in vals)
				v.Dispose ();
		}

		[GLib.Signal("transact_start")]
		public event RC.TransactStartHandler TransactStart {
			add {
				GLib.Signal sig = GLib.Signal.Lookup (this, "transact_start", new TransactStartSignalDelegate(TransactStartSignalCallback));
				sig.AddDelegate (value);
			}
			remove {
				GLib.Signal sig = GLib.Signal.Lookup (this, "transact_start", new TransactStartSignalDelegate(TransactStartSignalCallback));
				sig.RemoveDelegate (value);
			}
		}

		[GLib.CDeclCallback]
		delegate void DatabaseLockedVMDelegate (IntPtr packman);

		static DatabaseLockedVMDelegate DatabaseLockedVMCallback;

		static void databaselocked_cb (IntPtr packman)
		{
			Packman packman_managed = GLib.Object.GetObject (packman, false) as Packman;
			packman_managed.OnDatabaseLocked ();
		}

		private static void OverrideDatabaseLocked (GLib.GType gtype)
		{
			if (DatabaseLockedVMCallback == null)
				DatabaseLockedVMCallback = new DatabaseLockedVMDelegate (databaselocked_cb);
			OverrideVirtualMethod (gtype, "database_locked", DatabaseLockedVMCallback);
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
			foreach (GLib.Value v in vals)
				v.Dispose ();
		}

		[GLib.Signal("database_locked")]
		public event System.EventHandler DatabaseLocked {
			add {
				GLib.Signal sig = GLib.Signal.Lookup (this, "database_locked");
				sig.AddDelegate (value);
			}
			remove {
				GLib.Signal sig = GLib.Signal.Lookup (this, "database_locked");
				sig.RemoveDelegate (value);
			}
		}

		[GLib.CDeclCallback]
		delegate void TransactProgressSignalDelegate (IntPtr arg0, UIntPtr arg1, UIntPtr arg2, IntPtr gch);

		static void TransactProgressSignalCallback (IntPtr arg0, UIntPtr arg1, UIntPtr arg2, IntPtr gch)
		{
			GLib.Signal sig = ((GCHandle) gch).Target as GLib.Signal;
			if (sig == null)
				throw new Exception("Unknown signal GC handle received " + gch);

			RC.TransactProgressArgs args = new RC.TransactProgressArgs ();
			args.Args = new object[2];
			args.Args[0] = (ulong) arg1;
			args.Args[1] = (ulong) arg2;
			RC.TransactProgressHandler handler = (RC.TransactProgressHandler) sig.Handler;
			handler (GLib.Object.GetObject (arg0), args);

		}

		[GLib.CDeclCallback]
		delegate void TransactProgressVMDelegate (IntPtr packman, UIntPtr amount, UIntPtr total);

		static TransactProgressVMDelegate TransactProgressVMCallback;

		static void transactprogress_cb (IntPtr packman, UIntPtr amount, UIntPtr total)
		{
			Packman packman_managed = GLib.Object.GetObject (packman, false) as Packman;
			packman_managed.OnTransactProgress ((ulong) amount, (ulong) total);
		}

		private static void OverrideTransactProgress (GLib.GType gtype)
		{
			if (TransactProgressVMCallback == null)
				TransactProgressVMCallback = new TransactProgressVMDelegate (transactprogress_cb);
			OverrideVirtualMethod (gtype, "transact_progress", TransactProgressVMCallback);
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
			foreach (GLib.Value v in vals)
				v.Dispose ();
		}

		[GLib.Signal("transact_progress")]
		public event RC.TransactProgressHandler TransactProgress {
			add {
				GLib.Signal sig = GLib.Signal.Lookup (this, "transact_progress", new TransactProgressSignalDelegate(TransactProgressSignalCallback));
				sig.AddDelegate (value);
			}
			remove {
				GLib.Signal sig = GLib.Signal.Lookup (this, "transact_progress", new TransactProgressSignalDelegate(TransactProgressSignalCallback));
				sig.RemoveDelegate (value);
			}
		}

		[GLib.CDeclCallback]
		delegate void DatabaseUnlockedVMDelegate (IntPtr packman);

		static DatabaseUnlockedVMDelegate DatabaseUnlockedVMCallback;

		static void databaseunlocked_cb (IntPtr packman)
		{
			Packman packman_managed = GLib.Object.GetObject (packman, false) as Packman;
			packman_managed.OnDatabaseUnlocked ();
		}

		private static void OverrideDatabaseUnlocked (GLib.GType gtype)
		{
			if (DatabaseUnlockedVMCallback == null)
				DatabaseUnlockedVMCallback = new DatabaseUnlockedVMDelegate (databaseunlocked_cb);
			OverrideVirtualMethod (gtype, "database_unlocked", DatabaseUnlockedVMCallback);
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
			foreach (GLib.Value v in vals)
				v.Dispose ();
		}

		[GLib.Signal("database_unlocked")]
		public event System.EventHandler DatabaseUnlocked {
			add {
				GLib.Signal sig = GLib.Signal.Lookup (this, "database_unlocked");
				sig.AddDelegate (value);
			}
			remove {
				GLib.Signal sig = GLib.Signal.Lookup (this, "database_unlocked");
				sig.RemoveDelegate (value);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_packman_verify(IntPtr raw, IntPtr package, uint type);

		public GLib.SList Verify(RC.Package package, uint type) {
			IntPtr raw_ret = rc_packman_verify(Handle, package == null ? IntPtr.Zero : package.Handle, type);
			GLib.SList ret = new GLib.SList(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_packman_patch_parents(IntPtr raw, IntPtr package);

		public GLib.SList PatchParents(RC.Package package) {
			IntPtr raw_ret = rc_packman_patch_parents(Handle, package == null ? IntPtr.Zero : package.Handle);
			GLib.SList ret = new GLib.SList(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern int rc_packman_version_compare(IntPtr raw, IntPtr spec1, IntPtr spec2);

		public int VersionCompare(RC.PackageSpec spec1, RC.PackageSpec spec2) {
			int raw_ret = rc_packman_version_compare(Handle, spec1 == null ? IntPtr.Zero : spec1.Handle, spec2 == null ? IntPtr.Zero : spec2.Handle);
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
			IntPtr raw_ret = rc_packman_file_list(Handle, package == null ? IntPtr.Zero : package.Handle);
			GLib.SList ret = new GLib.SList(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_packman_query(IntPtr raw, IntPtr name);

		public GLib.SList Query(string name) {
			IntPtr name_as_native = GLib.Marshaller.StringToPtrGStrdup (name);
			IntPtr raw_ret = rc_packman_query(Handle, name_as_native);
			GLib.SList ret = new GLib.SList(raw_ret);
			GLib.Marshaller.Free (name_as_native);
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
		static extern IntPtr rc_packman_find_file(IntPtr raw, IntPtr filename);

		public GLib.SList FindFile(string filename) {
			IntPtr filename_as_native = GLib.Marshaller.StringToPtrGStrdup (filename);
			IntPtr raw_ret = rc_packman_find_file(Handle, filename_as_native);
			GLib.SList ret = new GLib.SList(raw_ret);
			GLib.Marshaller.Free (filename_as_native);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_packman_get_file_extension(IntPtr raw);

		public string FileExtension { 
			get {
				IntPtr raw_ret = rc_packman_get_file_extension(Handle);
				string ret = GLib.Marshaller.Utf8PtrToString (raw_ret);
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
		static extern bool rc_packman_parse_version(IntPtr raw, IntPtr input, out bool has_epoch, out uint epoch, out IntPtr version, out IntPtr release);

		public bool ParseVersion(string input, out bool has_epoch, out uint epoch, out string version, out string release) {
			IntPtr input_as_native = GLib.Marshaller.StringToPtrGStrdup (input);
			IntPtr version_as_native;
			IntPtr release_as_native;
			bool raw_ret = rc_packman_parse_version(Handle, input_as_native, out has_epoch, out epoch, out version_as_native, out release_as_native);
			bool ret = raw_ret;
			GLib.Marshaller.Free (input_as_native);
			version = GLib.Marshaller.PtrToStringGFree(version_as_native);
			release = GLib.Marshaller.PtrToStringGFree(release_as_native);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_packman_get_reason(IntPtr raw);

		public string Reason { 
			get {
				IntPtr raw_ret = rc_packman_get_reason(Handle);
				string ret = GLib.Marshaller.Utf8PtrToString (raw_ret);
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
            ret = new RC.Package(raw_ret);
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
