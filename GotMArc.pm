package GotMArc;
use strict;
use warnings;
use v5.32;
use Exporter;

our @ISA = qw(Exporter);
our @EXPORT_OK = qw($logo san initpage endpage);

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
