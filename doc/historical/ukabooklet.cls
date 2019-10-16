%% 
%% This is file derived from`siggraph.cls',
%% 
%%
%%
%% NOTE:
%%  This is a beta release and hasn't been thoroughly tested yet. Please
%%  send me any bug reports, suggestions for improvement, or other comments.
%%  I will do my best to fix any problems before the Siggraph submission
%%  deadline, but I make no promises.  The likelihood of my responding to
%%  any given comment is directly proportional to the amount of time before
%%  the deadline that the comment is received.
%%
%% \CharacterTable
%%  {Upper-case    \A\B\C\D\E\F\G\H\I\J\K\L\M\N\O\P\Q\R\S\T\U\V\W\X\Y\Z
%%   Lower-case    \a\b\c\d\e\f\g\h\i\j\k\l\m\n\o\p\q\r\s\t\u\v\w\x\y\z
%%   Digits        \0\1\2\3\4\5\6\7\8\9
%%   Exclamation   \!     Double quote  \"     Hash (number) \#
%%   Dollar        \$     Percent       \%     Ampersand     \&
%%   Acute accent  \'     Left paren    \(     Right paren   \)
%%   Asterisk      \*     Plus          \+     Comma         \,
%%   Minus         \-     Point         \.     Solidus       \/
%%   Colon         \:     Semicolon     \;     Less than     \<
%%   Equals        \=     Greater than  \>     Question mark \?
%%   Commercial at \@     Left bracket  \[     Backslash     \\
%%   Right bracket \]     Circumflex    \^     Underscore    \_
%%   Grave accent  \`     Left brace    \{     Vertical bar  \|
%%   Right brace   \}     Tilde         \~}
%%
\NeedsTeXFormat{LaTeX2e}[1994/06/01]
\ProvidesClass{ukabooklet}
[1996/01/17 v1.1.1
 Siggraph proceedings document class]
\newif\ifacm@camera
\newif\ifacm@preprint
\newcommand{\acm@columnmode}{}
\newcommand{\acm@pointsize}{}
\newif\ifacm@doublespaced
\DeclareOption{cameraready}{%
    \acm@cameratrue%
    \acm@preprintfalse%
    \renewcommand{\acm@columnmode}{twocolumn}%
    \acm@doublespacedfalse%
    \renewcommand{\acm@pointsize}{9pt}}
\DeclareOption{review}{%
    \acm@camerafalse%
    \renewcommand{\acm@columnmode}{twocolumn}%
    \acm@doublespacedfalse%
    \renewcommand{\acm@pointsize}{9pt}}
\DeclareOption{widereview}{%
    \acm@camerafalse%
    \renewcommand{\acm@columnmode}{onecolumn}%
    \acm@doublespacedtrue%
    \renewcommand{\acm@pointsize}{12pt}}
\DeclareOption{preprint}{%
    \acm@preprinttrue}
\ExecuteOptions{cameraready}
\newcommand{\acm@optwarning}[1]{%
    \ifacm@camera
        \ClassWarningNoLine{ukabooklet}%
            {#1 option used in camera-ready mode.\MessageBreak
             This violates submission specifications}
    \fi
}
\DeclareOption{onecolumn}{%
    \acm@optwarning{onecolumn}
    \renewcommand{\acm@columnmode}{onecolumn}}
\DeclareOption{twocolumn}{%
    \renewcommand{\acm@columnmode}{twocolumn}}
\DeclareOption{9pt}{%
    \renewcommand{\acm@pointsize}{9pt}}
\DeclareOption{10pt}{%
    \acm@optwarning{10pt}
    \renewcommand{\acm@pointsize}{10pt}}
\DeclareOption{11pt}{%
    \acm@optwarning{11pt}
    \renewcommand{\acm@pointsize}{11pt}}
\DeclareOption{12pt}{%
    \acm@optwarning{12pt}
    \renewcommand{\acm@pointsize}{12pt}}
\DeclareOption{singlespace}{%
    \acm@doublespacedfalse}
\DeclareOption{doublespace}{%
    \acm@optwarning{doublespace}
    \acm@doublespacedtrue}
\newcount\acm@specversion
\DeclareOption{version1996}{\acm@specversion=1996}
\DeclareOption{version1994}{\acm@specversion=1994}
\ExecuteOptions{version1996}
\DeclareOption{titlepage}{%
    \OptionNotUsed%
    \ClassWarningNoLine{ukabooklet}{titlepage option not allowed}}
\DeclareOption{landscape}{%
    \OptionNotUsed%
    \ClassWarningNoLine{ukabooklet}{landscape option not allowed}}
\DeclareOption*{\PassOptionsToClass{\CurrentOption}{book}}
\ProcessOptions
\PassOptionsToClass{\acm@columnmode}{book}
\ifdim\acm@pointsize>9pt
    \PassOptionsToClass{\acm@pointsize}{book}
\fi
\LoadClass{book}
\newcommand{\@maketitle}{}
\newcommand{\acm@preprinttext}{To appear in the SIGGRAPH conference proceedings}
\newcommand{\preprinttext}[1]{\renewcommand{\acm@preprinttext}{#1}}
\newcommand{\ifcamera}[1]{\ifacm@camera #1 \fi}
\newcommand{\ifreview}[1]{\ifacm@camera \else #1 \fi}
\newcommand{\ifcameraelse}[2]{\ifacm@camera #1 \else #2 \fi}
\newcommand{\ifreviewelse}[2]{\ifacm@camera #2 \else #1 \fi}
\setlength{\textheight}{9.25in}
\setlength{\topmargin}{-0.700in}
\setlength{\headheight}{0.2in}
\setlength{\headsep}{0.250in}
\setlength{\footskip}{0.5in}
\flushbottom
\setlength{\textwidth}{7in}
\setlength{\oddsidemargin}{-0.25in}
\setlength{\evensidemargin}{-0.25in}
\setlength{\columnsep}{2pc}
\setlength{\parindent}{1em}
\renewcommand{\ps@plain}%
    {%
    \ifacm@preprint%
\renewcommand{\@oddhead}{\hfil\textit{\large\acm@preprinttext}\hfil}%
    \else%
\renewcommand{\@oddhead}{}%
    \fi%
    \ifacm@camera%
\ifacm@preprint%
    \renewcommand{\@oddfoot}{\hfil\textrm{\thepage}\hfil}%
\else%
    \renewcommand{\@oddfoot}{}%
\fi%
    \else%
\renewcommand{\@oddfoot}{\hfil\textrm{\thepage}\hfil}%
    \fi%
    \renewcommand{\@evenhead}{\@oddhead}%
    \renewcommand{\@evenfoot}{\@oddfoot}%
    }
\renewcommand{\ps@empty}%
    {%
    \ifacm@preprint%
\renewcommand{\@oddhead}{\hfil\textit{\large\acm@preprinttext}\hfil}%
    \else%
\renewcommand{\@oddhead}{}%
    \fi%
    \renewcommand{\@oddfoot}{}%
    \renewcommand{\@evenhead}{\@oddhead}%
    \renewcommand{\@evenfoot}{\@oddfoot}%
    }
\pagestyle{plain}
\newcommand{\acm@setninepoint}{
    \renewcommand\normalsize{%
        \@setfontsize\normalsize\@ixpt\@xpt
        \abovedisplayskip 9\p@ \@plus2\p@ \@minus4\p@
        \abovedisplayshortskip \z@ \@plus3\p@
        \belowdisplayshortskip 6\p@ \@plus3\p@ \@minus3\p@
        \belowdisplayskip \abovedisplayskip
        \let\@listi\@listI}
    \renewcommand\small{%
        \@setfontsize\small\@viipt\@ixpt
        \abovedisplayskip 8.5\p@ \@plus3\p@ \@minus4\p@
        \abovedisplayshortskip \z@ \@plus2\p@
        \belowdisplayshortskip 4\p@ \@plus2\p@ \@minus2\p@
        \def\@listi{\leftmargin\leftmargini
                    \topsep 4\p@ \@plus2\p@ \@minus2\p@
                    \parsep 2\p@ \@plus\p@ \@minus\p@
                    \itemsep \parsep}%
        \belowdisplayskip \abovedisplayskip}
    \renewcommand\footnotesize{%
        \@setfontsize\footnotesize\@viiipt{9.5}%
        \abovedisplayskip 6\p@ \@plus2\p@ \@minus4\p@
        \abovedisplayshortskip \z@ \@plus\p@
        \belowdisplayshortskip 3\p@ \@plus\p@ \@minus2\p@
        \def\@listi{\leftmargin\leftmargini
                    \topsep 3\p@ \@plus\p@ \@minus\p@
                    \parsep 2\p@ \@plus\p@ \@minus\p@
                    \itemsep \parsep}%
        \belowdisplayskip \abovedisplayskip}
    \renewcommand\scriptsize{\@setfontsize\scriptsize\@viipt\@viiipt}
    \renewcommand\tiny{\@setfontsize\tiny\@vpt\@vipt}
    \renewcommand\large{\@setfontsize\large\@xpt\@xiipt}
    \renewcommand\Large{\@setfontsize\Large\@xiipt{14}}
    \renewcommand\LARGE{\@setfontsize\LARGE\@xivpt{18}}
    \renewcommand\huge{\@setfontsize\huge\@xviipt{22}}
    \renewcommand\Huge{\@setfontsize\Huge\@xxpt{25}}
    \selectfont
}
\font\@chapfont=cmss17 scaled 2800
\ifdim\acm@pointsize=9pt
    \acm@setninepoint
\fi
\newlength\@chapnamelength
\def\@makechapterhead#1{%
  \vspace*{200\p@}%
%  \settowidth{\@chapnamelength}{\@chapfont #1}
%  \ifdim\@chapnamelength>\hsize
  \setlength{\@chapnamelength}{\hsize}
%  \fi
  {\raggedleft \normalfont
    \ifnum \c@secnumdepth >\m@ne
      \if@mainmatter
        \Huge\bfseries \@chapapp\space \thechapter
        \par\nobreak
        \vskip 40\p@
      \fi
    \fi
    \interlinepenalty\@M
    \marginpar{\hspace*{-\@chapnamelength}%
      \hspace*{-5\p@}\rule{\@chapnamelength}{.5\p@}}
    \vskip 10\p@
    \@chapfont #1\par\nobreak
    \vskip 40\p@
  }}
\newcommand{\acm@sectionfont}{}
\newcommand{\sectionfont}[1]{\renewcommand{\acm@sectionfont}{#1}}
\renewcommand\section{\@startsection {section}{1}{\z@}%
                {-3.5ex \@plus -1ex \@minus -.2ex}%
                {2.3ex \@plus.2ex}%
                {\reset@font\Large\sffamily\bfseries\acm@sectionfont}}
\renewcommand\subsection{\@startsection{subsection}{2}{\z@}%
                {-3.25ex\@plus -1ex \@minus -.2ex}%
                {1.5ex \@plus .2ex}%
                {\reset@font\large\sffamily\bfseries\acm@sectionfont}}
\renewcommand\subsubsection{\@startsection{subsubsection}{3}{\z@}%
                {-3.25ex\@plus -1ex \@minus -.2ex}%
                {1.5ex \@plus .2ex}%
                {\reset@font\normalsize\sffamily\bfseries\acm@sectionfont}}
\renewcommand\paragraph{\@startsection{paragraph}{4}{\z@}%
                {3.25ex \@plus1ex \@minus.2ex}%
                {-1em}%
                {\reset@font\normalsize\sffamily\bfseries\acm@sectionfont}}
\renewcommand\subparagraph{\@startsection{subparagraph}{5}{\parindent}%
                {3.25ex \@plus1ex \@minus .2ex}%
                {-1em}%
                {\reset@font\normalsize\sffamily\bfseries\acm@sectionfont}}
\let\acm@chaptername\chaptername
\let\acm@appendixname\appendixname
\let\acm@contentsname\contentsname
\let\acm@indexname\indexname
\def\chaptername{\sffamily\acm@chaptername}
\def\appendixname{\sffamily\acm@appendixname}
\def\contentsname{\sffamily\acm@contentsname}
\def\indexname{\sffamily\acm@indexname}
\newcommand{\acm@empty}{}
\newcommand{\acm@affiliation}{}
\newcommand{\affiliation}[1]{\renewcommand{\acm@affiliation}{#1}}
\newcommand{\acm@category}{}
\newcommand{\category}[1]{\renewcommand{\acm@category}{#1}}
\newcommand{\acmcategory}[1]{\category{#1}}
\newcommand{\acm@format}{}
\newcommand{\format}[1]{\renewcommand{\acm@format}{#1}}
\newcommand{\acmformat}[1]{\format{#1}}
\newcommand{\acm@teaser}{}
\newcommand{\teaser}[1]{\renewcommand{\acm@teaser}{#1}}
\newlength{\titlespace}
\setlength{\titlespace}{0.25in}
\newlength{\teaserspace}
\setlength{\teaserspace}{0.25in}
\ifacm@camera
    \renewcommand{\@maketitle}{%
        \begin{center}%
            {\LARGE\sffamily\bfseries\acm@sectionfont \@title \par}%
            \vspace{1\baselineskip}%
            \large
            \begin{tabular}[t]{c}%
                \@author
            \end{tabular}\par%
            \ifx\acm@affiliaton\acm@empty \else%
                \vspace{1\baselineskip}%
                \acm@affiliation\par%
            \fi%
            \vspace{\titlespace}%
            \ifx\acm@teaser\acm@empty \else%
                \begingroup%
                    \def\@captype{figure}%
                    \acm@teaser%
                \endgroup\par%
                \vspace{\teaserspace}%
            \fi%
        \end{center} \par%
    }
\else
    \renewcommand{\@maketitle}{%
        \begin{center}%
            \renewcommand{\thanks}[1]{}
            {\LARGE\sffamily\bfseries\acm@sectionfont \@title \par}%
            \vspace{1\baselineskip}%
            {Category: \acm@category \par}%
            \vspace{\titlespace}%
            \ifx\acm@teaser\acm@empty \else%
                \begingroup%
                    \def\@captype{figure}%
                    \acm@teaser%
                \endgroup\par%
                \vspace{\teaserspace}%
            \fi%
        \end{center} \par%
    }
\fi
\let\acm@origmaketitle\maketitle
\renewcommand{\maketitle}{%
    \let\acm@title\@title%
    \let\acm@author\@author%
    \acm@origmaketitle%
    \thispagestyle{empty}%
    \ifacm@doublespaced%
        \renewcommand{\baselinestretch}{1.66}\selectfont%
    \fi
}
\newtoks\acm@abs
\ifacm@camera\else
    \long\def\acm@add#1{\global\acm@abs\expandafter{\the\acm@abs#1}}
    \long\def\acm@collect{%
        \global\acm@abs{}%
        \let\abstract\acm@@collect
        \abstract
    }
    \long\def\acm@@collect#1\end#2{%
        \def\@tempa{#2}%
        \ifx\@tempa\@currenvir
            \acm@add{#1}%
            \edef\abstract{\noexpand\end{\@tempa}}%
        \else
            \acm@add{#1\end{#2}}%
        \fi
        \abstract
    }
    \newcommand{\acm@modify}{%
        \let\acm@origabs\abstract%
        \let\acm@origendabs\endabstract%
        \renewenvironment{abstract}%
            {\acm@collect}%
            {\begingroup
                \let\abstract\acm@origabs
                \let\endabstract\acm@origendabs
                \begin{abstract} \the\acm@abs \end{abstract}
            \endgroup}%
    }
    \AtBeginDocument{\acm@modify}
\fi
\newlength{\acm@copyrightlength}
\ifnum\acm@specversion=1994
    \setlength{\acm@copyrightlength}{1in}
\else
    \setlength{\acm@copyrightlength}{1.5in}
\fi
\newcommand{\copyrightspace}{%
    \renewcommand{\thefootnote}{}%
    \footnotetext[0]{\rule[\acm@copyrightlength]{2.71828in}{0in}}%
    \renewcommand{\thefootnote}{\arabic{footnote}}
}
\renewcommand{\footnoterule}{%
    \kern-3pt
    \hrule width 0.45\columnwidth
    \kern 2.6pt}
\newcommand{\acm@contactname}{}
\newcommand{\contactname}[1]{\renewcommand{\acm@contactname}{#1}}
\newcommand{\acm@contactaddress}{}
\newcommand{\contactaddress}[1]{\renewcommand{\acm@contactaddress}{#1}}
\newcommand{\acm@contactphone}{}
\newcommand{\contactphone}[1]{\renewcommand{\acm@contactphone}{#1}}
\newcommand{\acm@contactfax}{}
\newcommand{\contactfax}[1]{\renewcommand{\acm@contactfax}{#1}}
\newcommand{\acm@contactemail}{}
\newcommand{\contactemail}[1]{\renewcommand{\acm@contactemail}{#1}}
\newcommand{\passport}[1]{%
  \ClassWarningNoLine{ukabooklet}{%
    You no longer need to specify which author should \MessageBreak%
    receive a free conference passport when submitting \MessageBreak%
    papers for review.\MessageBreak%
    The passport command has therefore been disabled}}
\newcommand{\acmpassport}[1]{\passport{#1}}
\newcommand{\acm@keywords}{}
\newcommand{\keywords}[1]{\renewcommand{\acm@keywords}{#1}}
\newcommand{\acm@estpages}{}
\newcommand{\estpages}[1]{\renewcommand{\acm@estpages}{#1}}
\newif\ifacm@cover
\ifacm@camera
    \acm@coverfalse
\else
    \acm@covertrue
\fi
\newcommand{\suppresscover}{\acm@coverfalse}
\newcommand{\acm@coverpage}{%
    \begin{titlepage}%
        \def\thanks##1{}
        \acm@setninepoint\normalsize
        \renewcommand{\baselinestretch}{1}\selectfont
        \begin{center}%
            \vspace*{\fill}
            {\LARGE\sffamily\bfseries\acm@sectionfont \acm@title \par}%
            \vspace{2\baselineskip}%
            {\large
                \begin{tabular}[t]{c}%
                \acm@author
             \end{tabular}\par%
            }%
            \vspace{1\baselineskip}%
            {\large \acm@affiliation \par}%
            \addvspace{3\baselineskip}%
            {Category: \acm@category \par}%
            \vspace{0.5\baselineskip}%
            {Format: \acm@format \par}%
            \vspace{3\baselineskip}%
            \begin{tabular}{ll}
                Contact: & \acm@contactname \\[1\baselineskip]
                         & \begin{tabular}[b]{@{}l@{}}
                               \acm@contactaddress
                               \end{tabular} \\[1\baselineskip]
                phone: & \acm@contactphone \\
                fax: & \acm@contactfax \\
                email: & \acm@contactemail
            \end{tabular}\par%
            \vspace*{3\baselineskip}%
            {Estimated \# of pages: \acm@estpages \par}%
            \vspace*{\baselineskip}%
            {Keywords: \acm@keywords \par}%
            \vspace*{\fill}%
            \begin{minipage}{5in}%
                \the\acm@abs
            \end{minipage}\par%
            \vspace*{\fill}
        \end{center}%
    \end{titlepage}%
}
\AtEndDocument{\ifacm@cover \acm@coverpage \fi}
\newcommand{\acmopening}[1]{%
  \ClassError{ukabooklet}{%
    The acmopening command is no longer needed.\MessageBreak%
    Switch to the maketitle command and abstract environment}{}}
\InputIfFileExists{siggraph.cfg}
        {\typeout{***************************************^^J%
                  * Local config file siggraph.cfg used *^^J%
                  ***************************************}}
        {}
\endinput
%%
%% End of file `ukabooklet.cls'.

