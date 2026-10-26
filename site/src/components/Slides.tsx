import { useEffect, useState } from 'react'
import { HERO_SLIDES, HERO_SLIDE_TIME } from '../config'

// Imagens do fundo da home: troca com fade e zoom lento, em loop
export default function Slides() {
  const [current, setCurrent] = useState(0)

  useEffect(() => {
    if (HERO_SLIDES.length < 2) return
    const id = setInterval(() => setCurrent((c) => (c + 1) % HERO_SLIDES.length), HERO_SLIDE_TIME)
    return () => clearInterval(id)
  }, [])

  return (
    <div className="slides" aria-hidden="true">
      {HERO_SLIDES.map((src, i) => (
        <div
          key={src}
          className={`slide${i === current ? ' on' : ''}`}
          style={{ backgroundImage: `url(${src})`, animationDuration: `${HERO_SLIDE_TIME + 1500}ms` }}
        />
      ))}
    </div>
  )
}
