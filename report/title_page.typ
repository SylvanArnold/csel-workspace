#let custom-title-page(
  title: "Linux Embedded Systems",
  subtitle: "Lab Report",
  student: "Sylvan Arnold",
  program: "Computer Science",
  class_name: "MA_CSEL1",
  academic_year: "2026",
  repository: "https://github.com/SylvanArnold/csel-workspace",
) = [
  #align(center, [
    #image("ressources/hesso-logo.svg", width: 42mm)
    #v(12mm)
    #text(size: 33pt, weight: "bold", fill: rgb("#0f172a"))[#title]
    #v(4mm)
    #text(size: 16pt, fill: rgb("#334155"))[#subtitle]
    #v(9mm)
    #rect(width: 78%, height: 1pt, fill: rgb("#94a3b8"))
  ])

  #v(10mm)

  #block(
    fill: rgb("#f8fafc"),
    stroke: 0.7pt + rgb("#cbd5e1"),
    radius: 8pt,
    inset: 14pt,
  )[
    #table(
      columns: (35%, 65%),
      stroke: none,
      inset: (x: 6pt, y: 4pt),
      [Student], [#student],
      [Program], [#program],
      [Academic year], [#academic_year],
      [Class], [#class_name],
      [Repository], [https://github.com/SylvanArnold/csel-workspace],
    )
  ]

  #v(23mm)

  #align(center, text(size: 10pt, fill: rgb("#64748b"))[
    HES-SO // Spring 2026
  ])

  #pagebreak()
]

#custom-title-page()
