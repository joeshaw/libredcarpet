// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	public class PackageMatch : GLib.Opaque {

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_match_get_channel_id(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_match_set_channel_id(IntPtr raw, IntPtr cid);

		public string ChannelId { 
			get {
				IntPtr raw_ret = rc_package_match_get_channel_id(Handle);
				string ret = GLib.Marshaller.Utf8PtrToString (raw_ret);
				return ret;
			}
			set {
				IntPtr cid_as_native = GLib.Marshaller.StringToPtrGStrdup (value);
				rc_package_match_set_channel_id(Handle, cid_as_native);
				GLib.Marshaller.Free (cid_as_native);
			}
		}

		[DllImport("libredcarpet")]
		static extern bool rc_package_match_equal(IntPtr raw, IntPtr match2);

		public bool Equal(RC.PackageMatch match2) {
			bool raw_ret = rc_package_match_equal(Handle, match2 == null ? IntPtr.Zero : match2.Handle);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_match_get_glob(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_match_set_glob(IntPtr raw, IntPtr glob_str);

		public string Glob { 
			get {
				IntPtr raw_ret = rc_package_match_get_glob(Handle);
				string ret = GLib.Marshaller.Utf8PtrToString (raw_ret);
				return ret;
			}
			set {
				IntPtr glob_str_as_native = GLib.Marshaller.StringToPtrGStrdup (value);
				rc_package_match_set_glob(Handle, glob_str_as_native);
				GLib.Marshaller.Free (glob_str_as_native);
			}
		}

		[DllImport("libredcarpet")]
		static extern int rc_package_match_get_importance(IntPtr raw, out bool match_gteq);

		public RC.PackageImportance GetImportance(out bool match_gteq) {
			int raw_ret = rc_package_match_get_importance(Handle, out match_gteq);
			RC.PackageImportance ret = (RC.PackageImportance) raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_package_match_set_importance(IntPtr raw, int importance, bool match_gteq);

		public void SetImportance(RC.PackageImportance importance, bool match_gteq) {
			rc_package_match_set_importance(Handle, (int) importance, match_gteq);
		}

		[DllImport("libredcarpet")]
		static extern void rc_package_match_set_channel(IntPtr raw, IntPtr channel);

		public RC.Channel Channel { 
			set {
				rc_package_match_set_channel(Handle, value == null ? IntPtr.Zero : value.Handle);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_match_get_dep(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_match_set_dep(IntPtr raw, IntPtr dep);

		public RC.PackageDep Dep { 
			get {
				IntPtr raw_ret = rc_package_match_get_dep(Handle);
				RC.PackageDep ret = raw_ret == IntPtr.Zero ? null : (RC.PackageDep) GLib.Opaque.GetOpaque (raw_ret, typeof (RC.PackageDep), false);
				return ret;
			}
			set {
				rc_package_match_set_dep(Handle, value == null ? IntPtr.Zero : value.Handle);
			}
		}

		public PackageMatch(IntPtr raw) : base(raw) {}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_match_new();

		public PackageMatch () 
		{
			Raw = rc_package_match_new();
		}

		[DllImport("libredcarpet")]
		static extern void rc_package_match_free(IntPtr raw);

		protected override void Free (IntPtr raw)
		{
			rc_package_match_free (raw);
		}

#endregion
#region Customized extensions
#line 1 "PackageMatch.custom"
/* -*- Mode: csharp; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

//namespace {
    public void ToXml (System.Xml.XmlTextWriter writer)
    {
        writer.WriteStartElement ("match");

        if (this.ChannelId != null)
            writer.WriteElementString ("channel", this.ChannelId);

        if (this.Dep != null)
            this.Dep.ToXml (writer);

        if (this.Glob != null)
            writer.WriteElementString ("glob", this.Glob);

        bool gteq;
        PackageImportance imp = this.GetImportance (out gteq);
        if (imp != PackageImportance.Invalid) {
            writer.WriteStartElement ("importance");
            writer.WriteAttributeString ("gteq", gteq ? "1" : "0");
            writer.WriteString (Global.ImportanceToString (imp));
            writer.WriteEndElement ();
        }

        writer.WriteEndElement ();
    }

    private static PackageMatch ParseMatch (System.Xml.XmlTextReader reader)
    {
        // Make sure we're in right position
        reader.ReadStartElement ("match");

        PackageMatch match = new PackageMatch ();

        if (reader.LocalName == "channel")
            match.ChannelId = reader.ReadElementString ();

        if (reader.LocalName == "dep") {
            PackageDep dep = new PackageDep (reader);
            match.Dep = dep;
            //dep.Unref ();

            // Move reader to start of next element
            while (reader.Read ())
                if (reader.NodeType == System.Xml.XmlNodeType.Element)
                    break;
        }

        if (reader.LocalName == "glob")
            match.Glob = reader.ReadElementString ();

        if (reader.LocalName == "importance") {
            int gteq = 0;
            string gteq_str = reader["gteq"];
            if (gteq_str != null)
                gteq = System.Xml.XmlConvert.ToInt32 (gteq_str);

            PackageImportance imp = RC.Global.PackageImportanceFromString (reader.ReadElementString ());
            match.SetImportance (imp, Convert.ToBoolean (gteq));
        }

        return match;
    }

    private static void FromXml (System.Xml.XmlTextReader reader, PackageMatchDelegate callback)
    {
        reader.MoveToContent ();

        while (reader.Read ()) {
            if (reader.NodeType == System.Xml.XmlNodeType.Element && reader.LocalName == "match") {
                PackageMatch match = ParseMatch (reader);

                if (!callback (match))
                    break;
            }
        }

        reader.Close ();
    }

    public static void FromXml (System.IO.Stream stream, PackageMatchDelegate callback)
    {
        System.Xml.XmlTextReader reader = new System.Xml.XmlTextReader (stream);
        FromXml (reader, callback);
        reader.Close ();
        stream.Close ();
    }

    private class FromXmlHelper {
        private System.Collections.ArrayList list;

        public FromXmlHelper () {
            this.list = new System.Collections.ArrayList ();
        }

        public bool ForeachMatchCb (PackageMatch match) {
            this.list.Add (match);
            return true;
        }

        public PackageMatch[] Matches {
            get { return (PackageMatch[]) this.list.ToArray (typeof (PackageMatch)); }
        }
    }

    public static PackageMatch[] FromXml (System.IO.Stream stream)
    {
        FromXmlHelper helper = new FromXmlHelper ();
        FromXml (stream, new PackageMatchDelegate (helper.ForeachMatchCb));
        return helper.Matches;
    }

#endregion
	}
}
