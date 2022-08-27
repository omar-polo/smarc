package GotMArc;
use strict;
use warnings;
use v5.32;
use Exporter;

our @ISA = qw(Exporter);
our @EXPORT_OK = qw(san parse small_logo initpage endpage index_header);

sub san {
	my $str = shift;
	$str =~ s/&/\&amp;/g;
	$str =~ s/</\&lt;/g;
	$str =~ s/>/\&gt;/g;
	return $str;
}

sub ssan {
	my $str = shift;
	$str =~ s/\s+/ /g;
	$str =~ s/\s+$//;
	return san($str);
}

sub mid2path {
	my $mid = shift;
	$mid =~ s,_,__,g;
	$mid =~ s,/,_,g;
	return $mid;
}

sub parse {
	my ($indent, $fname, $mid, $date, $from, $subj) = m{
		^([^-]*)-			# the indent level
		([^ ]+)\s			# filename
		<([^>]+)>			# message id
		(\d{4}-\d\d-\d\d[ ]\d\d:\d\d)	# date
		<([^>]+)>			# from
		(.*)				# subject
	}x or die "can't parse: $_";

	my $level = length($indent);
	$level = 10 if $indent =~ m/\.\.\d+\.\./;

	$mid = mid2path($mid);
	$from = ssan($from);
	$subj = ssan($subj);

	return ($level, $fname, $mid, $date, $from, $subj);
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

sub small_logo {
	my $fh = shift;
	print $fh $small_logo;
}

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

1;
