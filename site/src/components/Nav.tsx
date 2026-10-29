import { useEffect, useState } from 'react'
import { Link, NavLink, useLocation } from 'react-router-dom'
import Brand from './Brand'
import { DOWNLOAD_URL } from '../config'
import { LANGS, useI18n } from '../i18n'

export default function Nav() {
  const { t, lang, setLang } = useI18n()
  const { pathname } = useLocation()
  const [scrolled, setScrolled] = useState(false)
  const [open, setOpen] = useState(false)

  useEffect(() => {
    const onScroll = () => setScrolled(window.scrollY > 40)
    onScroll()
    addEventListener('scroll', onScroll, { passive: true })
    return () => removeEventListener('scroll', onScroll)
  }, [])

  useEffect(() => setOpen(false), [pathname])

  // Na home a barra fica sempre transparente sobre o vídeo, como no Zenkai
  const solid = scrolled && pathname !== '/'

  return (
    <header className={`nav${solid ? ' nav-solid' : ''}${open ? ' nav-open' : ''}`}>
      <Link to="/" className="nav-brand" aria-label="HypeWorld, início">
        <Brand />
      </Link>

      <nav className="nav-links" aria-label="Principal">
        <NavLink to="/sobre">{t.navAbout}</NavLink>
        <NavLink to="/trabalhe-conosco">{t.navWork}</NavLink>
      </nav>

      <div className="nav-right">
        <div className="langs" role="group" aria-label="Idioma">
          {LANGS.map((l) => (
            <button
              key={l}
              type="button"
              className={l === lang ? 'active' : ''}
              aria-pressed={l === lang}
              onClick={() => setLang(l)}
            >
              {l.toUpperCase()}
            </button>
          ))}
        </div>
        <a className="btn-launcher" href={DOWNLOAD_URL}>
          {t.download}
        </a>
      </div>

      <button
        type="button"
        className="nav-burger"
        aria-label="Menu"
        aria-expanded={open}
        onClick={() => setOpen((o) => !o)}
      >
        <span />
        <span />
      </button>
    </header>
  )
}
