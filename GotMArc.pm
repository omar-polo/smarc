# GotMArc was written by Omar Polo <op@openbsd.org> and is placed in
# the public domain.  The author hereby disclaims copyright to this
# source code.

package GotMArc;
use strict;
use warnings;
use v5.32;
use Exporter;
use File::Basename;

our @ISA = qw(Exporter);
our @EXPORT_OK = qw(san urlencode parse initpage endpage index_header
    thread_header threntry);

sub san {
	my $str = shift;
	$str =~ s/&/\&amp;/g;
	$str =~ s/</\&lt;/g;
	$str =~ s/>/\&gt;/g;
	return $str;
}

sub urlencode {
	my $str = shift;
	unless (defined($str)) {
		my ($pkg, $file, $line) = caller 1;
		die "bad $pkg / $file:$line";
	}
	$str =~ s/([^-_~.A-Za-z0-9])/sprintf("%%%2X", ord($1))/ge;
	return $str;
}

sub ssan {
	my $str = shift;
	$str =~ s/\s+/ /g;
	$str =~ s/\s+$//;
	return san($str);
}

sub parse {
	my $line = shift;
	my ($indent, $fname, $date, $from, $subj) = $line =~ m{
		^([^-]*)-			# the indent level
		([^ ]+)\s			# filename
		(\d{4}-\d\d-\d\d[ ]\d\d:\d\d)	# date
		<([^>]+)>			# from
		(.*)				# subject
	}x or die "can't parse: $line";

	my $level = length($indent);
	$level = 10 if $indent =~ m/\.\.\d+\.\./;

	$from = ssan($from);
	$subj = ssan($subj);

	my ($time, $id) = split /\./, basename($fname);
	my $mid = "$time.$id";

	return {level => $level, fname => $fname,
	    mid => $mid, date => $date, from => $from, subj => $subj};
}

sub readall {
	my $path = shift;
	local $/ = undef;
	open my $fh, "<", $path or die "can't open $path: $!";
	<$fh>;
}

my $small_logo = readall "logo-small.html";
my $hdr = readall "head.html";
my $foot = readall "foot.html";
my $idxhdr = readall "index-header.html";

sub initpage {
	my ($fh, $title) = @_;
	say $fh $hdr =~ s/TITLE/$title/r;
}

sub endpage {
	my $fh = shift;
	say $fh $foot;
}

sub index_header {
	my ($fh, $page, $subtitle) = @_;
	my $html = $idxhdr =~ s/PAGE/$page/r;
	$html =~ s/SUBTITLE/$subtitle/;
	print $fh $html;
}

sub thread_header {
	my ($fh, $tid, $mid, $e) = @_;
	my @entries = @$e;

	my $enctid = urlencode $tid if defined $tid;
	my $encmid = urlencode $mid if defined $mid;

	print $fh "<header class='mail-header'>\n";

	print $fh "<p>";
	print $fh $small_logo;
	print $fh "<a href='/'>Index</a>";
	print $fh " | <a href='/thread/$enctid.html#$encmid'>Thread</a>"
	    if defined $enctid;
	print $fh " | <a href='/search'>Search</a>";
	print $fh "</p>\n";

	say $fh "<dl>";
	foreach my $entry (@entries) {
		my ($k, $v) = split /: /, $entry, 2;
		chomp $v;
		say $fh "<dt>$k:</dt><dd>$v</dd>";
	}
	say $fh "</dl>";

	say $fh "<p>Download raw <a href='/text/$encmid.txt'>body</a>.</p>"
	    if defined $encmid;

	say $fh "</header>\n";
}

sub threntry {
	my ($fh, $type, $base, $last_level, $mail, $cur) = @_;
	my $level = $mail->{level} - $base;

	say $fh "</ul></li>" x ($last_level - $level) if $last_level > $level;
	say $fh "<li><ul>" if $last_level < $level;

	my $encmid = urlencode $mail->{mid};

	print $fh "<li id='$encmid' class='mail'>";
	print $fh "<p class='mail-meta'>";
	print $fh "<time>$mail->{date}</time> ";
	print $fh "<span class='from'>$mail->{from}</span>";
	print $fh "<span class='colon'>:</span>";
	print $fh "</p>";
	print $fh "<p class='subject'>";

	my $subj = $mail->{subj};
	if (!defined($cur) || $mail->{mid} ne $cur->{mid}) {
		print $fh "<a href='/$type/$encmid.html'>$subj</a>";
	} else {
		print $fh "<span>$subj</span>";
	}

	print $fh "</p>";
	print $fh "</li>";

	return $level;
}

1;
