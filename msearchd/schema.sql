create virtual table email using fts5(mid UNINDEXED, from, date, subj, body,
	tokenize = 'porter unicode61 remove_diacritics 2');
