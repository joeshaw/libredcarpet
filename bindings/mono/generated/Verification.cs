// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	public class Verification : GLib.Opaque {

		[DllImport ("libredcarpetsharpglue-0")]
		extern static uint rcsharp_rc_verification_get_info_offset ();

		static uint info_offset = rcsharp_rc_verification_get_info_offset ();
		public string Info {
			get {
				unsafe {
					IntPtr* raw_ptr = (IntPtr*)(((byte*)Handle) + info_offset);
					return GLib.Marshaller.Utf8PtrToString ((*raw_ptr));
				}
			}
		}

		[DllImport ("libredcarpetsharpglue-0")]
		extern static uint rcsharp_rc_verification_get_type_offset ();

		static uint type_offset = rcsharp_rc_verification_get_type_offset ();
		public RC.VerificationType Type {
			get {
				unsafe {
					int* raw_ptr = (int*)(((byte*)Handle) + type_offset);
					return (RC.VerificationType) (*raw_ptr);
				}
			}
			set {
				unsafe {
					int* raw_ptr = (int*)(((byte*)Handle) + type_offset);
					*raw_ptr = (int) value;
				}
			}
		}

		[DllImport ("libredcarpetsharpglue-0")]
		extern static uint rcsharp_rc_verification_get_status_offset ();

		static uint status_offset = rcsharp_rc_verification_get_status_offset ();
		public RC.VerificationStatus Status {
			get {
				unsafe {
					int* raw_ptr = (int*)(((byte*)Handle) + status_offset);
					return (RC.VerificationStatus) (*raw_ptr);
				}
			}
			set {
				unsafe {
					int* raw_ptr = (int*)(((byte*)Handle) + status_offset);
					*raw_ptr = (int) value;
				}
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_verification_type_to_string(int type);

		public static string TypeToString(RC.VerificationType type) {
			IntPtr raw_ret = rc_verification_type_to_string((int) type);
			string ret = GLib.Marshaller.Utf8PtrToString (raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_verification_slist_free(IntPtr verification_list);

		public static void SlistFree(GLib.SList verification_list) {
			rc_verification_slist_free(verification_list.Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_verification_set_keyring(IntPtr keyring);

		public static string Keyring { 
			set {
				IntPtr keyring_as_native = GLib.Marshaller.StringToPtrGStrdup (value);
				rc_verification_set_keyring(keyring_as_native);
				GLib.Marshaller.Free (keyring_as_native);
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_verification_cleanup();

		public static void Cleanup() {
			rc_verification_cleanup();
		}

		public Verification(IntPtr raw) : base(raw) {}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_verification_new();

		public Verification () 
		{
			Raw = rc_verification_new();
		}

		[DllImport("libredcarpet")]
		static extern void rc_verification_free(IntPtr raw);

		protected override void Free (IntPtr raw)
		{
			rc_verification_free (raw);
		}

#endregion
	}
}
