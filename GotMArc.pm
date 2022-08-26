package GotMArc;
use strict;
use warnings;
use v5.32;
use Exporter;

our @ISA = qw(Exporter);
our @EXPORT_OK = qw($logo san parse initpage endpage);

our $logo = <<'EOF';
<a href="https://gameoftrees.org" target="_blank">
  <img srcset='/got-tiny.png, /got-tiny@2x.png 2x'
       src='/got-tiny.png'
       width='64' height='39'
       alt='"GOT", but the "O" is a cute, smiling sun' /></a>
EOF

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

my $hdr = do {
	local $/ = undef;
	open my $fh, "<", "head.html"
	    or die "can't open head.html: $!";
	<$fh>;
};

sub initpage {
	my ($fh, $title) = @_;
	say $fh $hdr =~ s/TITLE/$title/r;
}

my $foot = do {
	local $/ = undef;
	open my $fh, "<", "foot.html"
	    or die "can't open foot.html: $!";
	<$fh>;
};

sub endpage {
	my $fh = shift;
	say $fh $foot;
}

1;
