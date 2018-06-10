#pragma once
/*
 * RetroShare Content Search and Indexing.
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <vector>
#include <xapian.h>

#include "retroshare/rsgxschannels.h"

struct DeepSearch
{
	//DeepSearch(const std::string& dbPath) : mDbPath(dbPath) {}

	struct SearchResult
	{
		// TODO: Use RsUrl from extra_locators branch instead of plain string
		std::string mUrl;
		std::string mSnippet;
	};

	/**
	 * @return search results count
	 */
	static uint32_t search( const std::string& queryStr,
	                        std::vector<SearchResult>& results,
	                        uint32_t maxResults = 100 )
	{
		results.clear();

		// Open the database we're going to search.
		Xapian::Database db(mDbPath);

		// Set up a QueryParser with a stemmer and suitable prefixes.
		Xapian::QueryParser queryparser;
		//queryparser.set_stemmer(Xapian::Stem("en"));
		queryparser.set_stemming_strategy(queryparser.STEM_SOME);
		// Start of prefix configuration.
		//queryparser.add_prefix("title", "S");
		//queryparser.add_prefix("description", "XD");
		// End of prefix configuration.

		// And parse the query.
		Xapian::Query query = queryparser.parse_query(queryStr);

		// Use an Enquire object on the database to run the query.
		Xapian::Enquire enquire(db);
		enquire.set_query(query);

		Xapian::MSet mset = enquire.get_mset(0, maxResults);

		for ( Xapian::MSetIterator m = mset.begin(); m != mset.end(); ++m )
		{
			const Xapian::Document& doc = m.get_document();

			SearchResult s;
			s.mUrl = doc.get_value(URL_VALUENO);
			s.mSnippet = mset.snippet(doc.get_data());
			results.push_back(s);
		}

		return results.size();
	}


	static void indexChannelGroup(const RsGxsChannelGroup& chan)
	{
		Xapian::WritableDatabase db(mDbPath, Xapian::DB_CREATE_OR_OPEN);

		// Set up a TermGenerator that we'll use in indexing.
		Xapian::TermGenerator termgenerator;
		//termgenerator.set_stemmer(Xapian::Stem("en"));

		// We make a document and tell the term generator to use this.
		Xapian::Document doc;
		termgenerator.set_document(doc);

		// Index each field with a suitable prefix.
		termgenerator.index_text(chan.mMeta.mGroupName, 1, "G");
		termgenerator.index_text(chan.mDescription, 1, "XD");

		// Index fields without prefixes for general search.
		termgenerator.index_text(chan.mMeta.mGroupName);
		termgenerator.increase_termpos();
		termgenerator.index_text(chan.mDescription);

		std::string rsLink("retroshare://channel?id=");
		rsLink += chan.mMeta.mGroupId.toStdString();

		// store the RS link so we are able to retrive it on matching search
		doc.add_value(URL_VALUENO, rsLink);

		// Store some fields for display purposes.
		doc.set_data(chan.mMeta.mGroupName + "\n" + chan.mDescription);

		// We use the identifier to ensure each object ends up in the
		// database only once no matter how many times we run the
		// indexer. "Q" prefix is a Xapian convention for unique id term.
		const std::string idTerm("Q" + rsLink);
		doc.add_boolean_term(idTerm);
		db.replace_document(idTerm, doc);
	}

	static void removeChannelFromIndex(RsGxsGroupId grpId)
	{
		// "Q" prefix is a Xapian convention for unique id term.
		std::string idTerm("Qretroshare://channel?id=");
		idTerm += grpId.toStdString();

		Xapian::WritableDatabase db(mDbPath, Xapian::DB_CREATE_OR_OPEN);
		db.delete_document(idTerm);
	}

	static void indexChannelPost(const RsGxsChannelPost& post)
	{
		Xapian::WritableDatabase db(mDbPath, Xapian::DB_CREATE_OR_OPEN);

		// Set up a TermGenerator that we'll use in indexing.
		Xapian::TermGenerator termgenerator;
		//termgenerator.set_stemmer(Xapian::Stem("en"));

		// We make a document and tell the term generator to use this.
		Xapian::Document doc;
		termgenerator.set_document(doc);

		// Index each field with a suitable prefix.
		termgenerator.index_text(post.mMeta.mMsgName, 1, "S");
		termgenerator.index_text(post.mMsg, 1, "XD");

		// Index fields without prefixes for general search.
		termgenerator.index_text(post.mMeta.mMsgName);
		termgenerator.increase_termpos();
		termgenerator.index_text(post.mMsg);

		// We use the identifier to ensure each object ends up in the
		// database only once no matter how many times we run the
		// indexer.
		std::string idTerm("Qretroshare://channel?id=");
		idTerm += post.mMeta.mGroupId.toStdString();
		idTerm += "&msgid=";
		idTerm += post.mMeta.mMsgId.toStdString();
		doc.add_boolean_term(idTerm);
		db.replace_document(idTerm, doc);
	}

private:

	enum : Xapian::valueno
	{
		/// Used to store retroshare url of indexed documents
		URL_VALUENO,

		/// @see Xapian::BAD_VALUENO
		BAD_VALUENO = Xapian::BAD_VALUENO
	};

	static std::string mDbPath;
};

std::string DeepSearch::mDbPath = "/tmp/deep_search_xapian_db";
