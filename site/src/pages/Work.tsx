import BigTitle from '../components/BigTitle'
import Sticker from '../components/Sticker'
import { useI18n } from '../i18n'

export default function Work() {
  const { t } = useI18n()

  return (
    <>
      <BigTitle kicker={t.workKicker} big={t.workBig} />
      <div className="page">
        <article className="prose">
          <Sticker>{t.workSticker}</Sticker>
          <p className="prose-sub">{t.workSub}</p>
          <p>{t.workP1}</p>
          <p className="prose-em">{t.workP2}</p>
        </article>
      </div>
    </>
  )
}
