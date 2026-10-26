import BigTitle from '../components/BigTitle'
import Sticker from '../components/Sticker'
import { useI18n } from '../i18n'

export default function About() {
  const { t } = useI18n()

  return (
    <>
      <BigTitle kicker={t.aboutKicker} big={t.aboutBig} />
      <div className="page">
        <ul className="values">
          {t.values.map((v, i) => (
            <li key={v}>
              <span className={`num n${i + 1}`}>{String(i + 1).padStart(2, '0')}</span>
              <span className="dash">-</span>
              {v}
            </li>
          ))}
        </ul>

        <article className="prose">
          <Sticker>{t.aboutSticker}</Sticker>
          <p className="prose-sub">{t.aboutSub}</p>
          <p>
            {t.aboutP1}
            <br />
            {t.aboutP2}
          </p>
          <p className="prose-em">{t.aboutVision}</p>
          <ul className="prose-list">
            {t.aboutList.map((l) => (
              <li key={l}>{l}</li>
            ))}
          </ul>
        </article>
      </div>
    </>
  )
}
