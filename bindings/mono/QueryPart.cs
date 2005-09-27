/*
 * Copyright (c) 2005 Novell, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, contact Novell, Inc.
 *
 * To contact Novell about this file by physical or electronic mail, 
 * you may find current contact information at www.novell.com.
 *
 */


using System;

namespace RC {

    public enum QueryType {
        Equal,
        NotEqual,
        Contains,
        NotContains,
        ContainsWord,
        NotContainsWord,
        GreaterThan,
        LessThan,
        GreaterThanOrEqualTo,
        LessThanOrEqualTo,
        BeginOr,
        EndOr,
        Invalid
    }
    
    [Serializable]    
    public struct QueryPart {
        public string Key;
        public QueryType Type;
        public string QueryStr;

        public static readonly QueryPart BeginOr = new QueryPart (null, QueryType.BeginOr, null);
        public static readonly QueryPart EndOr = new QueryPart (null, QueryType.EndOr, null);

        public QueryPart (string key, QueryType type, string queryStr) {
            this.Key = key;
            this.Type = type;
            this.QueryStr = queryStr;
        }

        public QueryPart (string key, string type, string queryStr) {
            this.Key = key;
            this.Type = QueryTypeFromString (type);
            this.QueryStr = queryStr;
        }

        public static QueryType QueryTypeFromString (string typeStr) {
            switch (typeStr) {
            case "==":
            case "=":
            case "is":
            case "eq":
                return QueryType.Equal;
            case "!=":
            case "is not":
            case "ne":
                return QueryType.NotEqual;
            case "contains":
                return QueryType.Contains;
            case "contains_word":
                return QueryType.ContainsWord;
            case "!contains":
                return QueryType.NotContains;
            case "!contains_word":
                return QueryType.NotContainsWord;
            case ">":
            case "gt":
                return QueryType.GreaterThan;
            case "<":
            case "lt":
                return QueryType.LessThan;
            case ">=":
            case "gteq":
                return QueryType.GreaterThanOrEqualTo;
            case "<=":
            case "lteq":
                return QueryType.LessThanOrEqualTo;
            case "begin-or":
                return QueryType.BeginOr;
            case "end-or":
                return QueryType.EndOr;
            default:
                throw new ArgumentException ();
            }
        }

        public override string ToString () {
            return String.Format ("[{0}, {1}, {2}]", this.Key, this.Type, this.QueryStr);
        }
    }

}
