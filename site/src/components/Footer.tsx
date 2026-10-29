import { FaDiscord, FaTiktok, FaTwitch, FaXTwitter, FaYoutube } from 'react-icons/fa6'
import Brand from './Brand'
import { SOCIAL } from '../config'
import { useI18n } from '../i18n'

export default function Footer() {
  const { t } = useI18n()
  const links = [t.legal, t.contact, t.terms, t.privacy]
  const social = [
    { href: SOCIAL.discord, label: 'Discord', Icon: FaDiscord },
    { href: SOCIAL.x, label: 'X', Icon: FaXTwitter },
    { href: SOCIAL.youtube, label: 'YouTube', Icon: FaYoutube },
    { href: SOCIAL.tiktok, label: 'TikTok', Icon: FaTiktok },
    { href: SOCIAL.twitch, label: 'Twitch', Icon: FaTwitch },
  ]

  return (
    <footer className="footer">
      <div className="footer-inner">
        <div className="footer-main">
          <div className="footer-row">
            <Brand size="lg" />
            <nav className="footer-links" aria-label="Rodapé">
              {links.map((l) => (
                <a key={l} href="#">
                  {l}
                </a>
              ))}
            </nav>
          </div>
          <p className="footer-copy">© 2026 - HypeWorld. {t.rights}</p>
        </div>
        <div className="footer-social">
          <span>{t.social}</span>
          <div>
            {social.map(({ href, label, Icon }) => (
              <a key={label} href={href} aria-label={label} target="_blank" rel="noreferrer">
                <Icon />
              </a>
            ))}
          </div>
        </div>
      </div>
    </footer>
  )
}
