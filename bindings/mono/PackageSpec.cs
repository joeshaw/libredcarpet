namespace RC {

    using System;
    using System.Collections;
    using System.Runtime.InteropServices;

    public sealed class PackageSpec : GLib.IWrapper, ICloneable, IComparable, IDisposable {

        IntPtr raw;
        bool owned;

        public PackageSpec (IntPtr raw, bool owned) {
            this.raw = raw;
            this.owned = owned;
        }

        public PackageSpec (IntPtr raw) : this (raw, false) {}

        [DllImport("libredcarpet")]
        static extern IntPtr rc_package_spec_new ();

        public PackageSpec () : this (rc_package_spec_new (), true) {}


        [DllImport("libredcarpet")]
        static extern IntPtr rc_package_spec_get_name (IntPtr raw);

        [DllImport("libredcarpet")]
        static extern void rc_package_spec_set_name (IntPtr raw, IntPtr value);

        public string Name { 
            get {
                IntPtr raw_ret = rc_package_spec_get_name (Handle);
                string ret = GLib.Marshaller.Utf8PtrToString (raw_ret);
                return ret;
            }
            set {
                IntPtr value_as_native = GLib.Marshaller.StringToPtrGStrdup (value);
                rc_package_spec_set_name (Handle, value_as_native);
                GLib.Marshaller.Free (value_as_native);
            }
        }

        [DllImport("libredcarpet")]
        static extern IntPtr rc_package_spec_get_version (IntPtr raw);

        [DllImport("libredcarpet")]
        static extern void rc_package_spec_set_version (IntPtr raw, IntPtr value);

        public string Version {
            get  {
                IntPtr raw_ret = rc_package_spec_get_version (Handle);
                string ret = GLib.Marshaller.Utf8PtrToString (raw_ret);
                return ret;
            }
            set  {
                IntPtr value_as_native = GLib.Marshaller.StringToPtrGStrdup (value);
                rc_package_spec_set_version (Handle, value_as_native);
                GLib.Marshaller.Free (value_as_native);
            }
        }

        [DllImport("libredcarpet")]
        static extern IntPtr rc_package_spec_get_release (IntPtr raw);

        [DllImport("libredcarpet")]
        static extern void rc_package_spec_set_release (IntPtr raw, IntPtr value);

        public string Release {
            get  {
                IntPtr raw_ret = rc_package_spec_get_release (Handle);
                string ret = GLib.Marshaller.Utf8PtrToString (raw_ret);
                return ret;
            }
            set  {
                IntPtr value_as_native = GLib.Marshaller.StringToPtrGStrdup (value);
                rc_package_spec_set_release (Handle, value_as_native);
                GLib.Marshaller.Free (value_as_native);
            }
        }

        [DllImport("libredcarpet")]
        static extern bool rc_package_spec_has_epoch (IntPtr raw);

        public bool HasEpoch { 
            get { return rc_package_spec_has_epoch (Handle); }
        }

        [DllImport("libredcarpet")]
        static extern int rc_package_spec_get_epoch (IntPtr raw);

        [DllImport("libredcarpet")]
        static extern void rc_package_spec_set_epoch (IntPtr raw, int value);

        public int Epoch {
            get  {
                int raw_ret = rc_package_spec_get_epoch (Handle);
                int ret = raw_ret;
                return ret;
            }
            set  { rc_package_spec_set_epoch (Handle, value); }
        }

        [DllImport("libredcarpet")]
        static extern int rc_package_spec_get_arch (IntPtr raw);

        [DllImport("libredcarpet")]
        static extern void rc_package_spec_set_arch (IntPtr raw, int value);

        public RC.Arch Arch {
            get  {
                int raw_ret = rc_package_spec_get_arch (Handle);
                RC.Arch ret = (RC.Arch) raw_ret;
                return ret;
            }
            set  { rc_package_spec_set_arch(Handle, (int) value); }
        }

        [DllImport("libredcarpet")]
        static extern IntPtr rc_package_spec_to_str (IntPtr raw);

        public override string ToString () {
            IntPtr raw_ret = rc_package_spec_to_str (Handle);
            string ret = GLib.Marshaller.PtrToStringGFree (raw_ret);
            return ret;
        }

        [DllImport("libredcarpet")]
        static extern IntPtr rc_package_spec_version_to_str (IntPtr raw);

        public string VersionToString () {
            IntPtr raw_ret = rc_package_spec_version_to_str (Handle);
            string ret = GLib.Marshaller.PtrToStringGFree (raw_ret);
            return ret;
        }

        [DllImport("libredcarpet")]
        static extern int rc_package_spec_equal(IntPtr a, IntPtr b);

        public override bool Equals (object other) {
            int ret = rc_package_spec_equal (Handle, ((PackageSpec) other).Handle);
            if (ret == 0)
                return false;

            return true;
        }

        public override int GetHashCode () {
            return ToString ().GetHashCode ();
        }

        // GLib.IWrapper
        public IntPtr Handle {
            get { return raw; }
        }

        ~PackageSpec () {
            Dispose ();
        }

        // IDisposable

        [DllImport("libredcarpet")]
        static extern void rc_package_spec_free (IntPtr raw);

        public void Dispose () {
            if (owned)
                rc_package_spec_free (Handle);
            GC.SuppressFinalize (this);
        }

        // ICloneable

        [DllImport("libredcarpet")]
        static extern void rc_package_spec_copy (IntPtr raw, IntPtr old);

        public object Clone () {
            PackageSpec copy = new PackageSpec ();
            rc_package_spec_copy (copy.Handle, Handle);
            return copy;
        }

        // IComparable

        public int CompareTo (object obj) {
            if (obj is PackageSpec) {
                PackageSpec other = (PackageSpec) obj;
                return rc_package_spec_equal (Handle, other.Handle);
            }

            throw new ArgumentException ("object is not a RC.PackageSpec");
        }
    }
}
